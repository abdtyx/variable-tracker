#include "pin.H"

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string>

#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <sched.h>

#include <boost/type_index.hpp>

#include "dma_tbl.hh"
#include "types.hh"

#define DEFAULT_NAME "__DEFAULT_NAME__"
#define DEFAULT_DELIMITER "->"

using std::cout, std::endl, std::vector, std::set, std::map, std::string;

namespace vt {

/////////////////////////////////////////
////////////////// var //////////////////
/////////////////////////////////////////
template <typename T>
struct compare_function {
    bool operator()(const T v1, const T v2) const {
        return v1->address < v2->address;
    }
};

struct var {
    string name;                                                        // 变量名
    void* address;                                                      // 变量地址
    bool invalid;                                                       // 预留
    set<var*, compare_function<var*> > father;                          // who points at this variable
    vector<var*> children;                                              // children存储了这个指针指向的所有变量的地址
    void (*log_read)(var* v);                                           // 记录读到的值的回调函数
    void (*log_before_write)(var* v);                                   // 记录写之前变量值的回调函数
    void (*log_after_write)(var* v);                                    // 记录写之后变量值的回调函数
    void (*cvs_before_write)(var* v);                                   // 写之前更新CVS的回调函数
    void (*cvs_after_write)(var* v, string delimiter, void* address);   // 写之后更新CVS的回调函数

    // compare function
    bool operator()(const var* v1, const var* v2) const {
        return v1->address < v2->address;
    }
};

void print_var(var* v, string space = "") {
    cout << space << "var {name: " << v->name << ", address: " << v->address << ", invalid: " << v->invalid << ", reference_counter: " << v->father.size() << "}" << endl;
    cout << space << "children {" << endl;
    space.push_back('\t');
    for (auto i : v->children) {
        print_var(i, space);
    }
    space.pop_back();
    cout << space << "}" << endl;
}

////////////////////////////////////////////////////
////////////////// ptr validation //////////////////
////////////////////////////////////////////////////
set<var*, compare_function<var*> > var_set;
uint64_t base_address = 0;
uint64_t stack_start = 0;
uint64_t rsp = 0;   // rsp is typically lower than stack_start

bool valid_ptr(void* addr) {
    // cout << "[validate]: " << addr << endl;
    uint64_t value = (uint64_t)addr;
    // value >>= 40;
    // cout << "[value]: " << value << endl;
    return (dma_tbl_find(addr) || (value >= rsp && value < stack_start));
}

bool invalid_ptr(void* addr) {
    return !valid_ptr(addr);
}

///////////////////////////////////////////////
////////////////// spin-lock //////////////////
///////////////////////////////////////////////
atomic_flag lock = ATOMIC_FLAG_INIT;
atomic_flag malloc_lock = ATOMIC_FLAG_INIT;

void lock_acquire() {
    while (atomic_flag_test_and_set(&lock))
        sched_yield();
}

void lock_release() {
    atomic_flag_clear(&lock);
}

void malloc_lock_acquire() {
    while (atomic_flag_test_and_set(&malloc_lock)) {
        lock_release();
        sched_yield();
        lock_acquire();
    }
}

void malloc_lock_release() {
    atomic_flag_clear(&malloc_lock);
}

/////////////////////////////////////////
////////////////// log //////////////////
/////////////////////////////////////////
template <typename T>
struct log {
    static void log_read(var* v) {
        T value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] " << boost::typeindex::type_id<T>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
    static void log_before_write(var* v) {
        T value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] " << boost::typeindex::type_id<T>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
    static void log_after_write(var* v) {
        T value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] " << boost::typeindex::type_id<T>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
};

template <>
struct log<void> {
    static void log_read(var* v) {}
    static void log_before_write(var* v) {}
    static void log_after_write(var* v) {}
};

template <>
struct log<char> {
    static void log_read(var* v) {
        char value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] " << boost::typeindex::type_id<char>().pretty_name() << ' ' << v->name << ' ' << (int)value << endl;
    }
    static void log_before_write(var* v) {
        char value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] " << boost::typeindex::type_id<char>().pretty_name() << ' ' << v->name << ' ' << (int)value << endl;
    }
    static void log_after_write(var* v) {
        char value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] " << boost::typeindex::type_id<char>().pretty_name() << ' ' << v->name << ' ' << (int)value << endl;
    }
};

template <typename T>
struct log<T*> {
    static void log_read(var* v) {
        void* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] " << boost::typeindex::type_id<T*>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
    static void log_before_write(var* v) {
        void* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] " << boost::typeindex::type_id<T*>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
    static void log_after_write(var* v) {
        void* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] " << boost::typeindex::type_id<T*>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
};

void cvs_before_write(var* v) {
    for (auto child : v->children) {
        child->father.erase(v);
        if (child->father.size() == 0) {
            child->cvs_before_write(child);
            // cout << var_set.size() << endl;
            var_set.erase(child);
            // cout << var_set.size() << endl;
            delete child;
        }
    }
    v->children.clear();
}

/////////////////////////////////////////
////////////////// cvs //////////////////
/////////////////////////////////////////
template <typename T>
struct cvs {
    static void cvs_after_write(var* v, string delimiter, void* address){}
};

template <typename T>
struct core_cvs {
    static void core_cvs_before_write(var* v){}
    static void core_cvs_after_write(var* v, string delimiter, void* address){}
};

template <>
struct core_cvs<void*> {
    static void core_cvs_before_write(var* v){}
    static void core_cvs_after_write(var* v, string delimiter, void* address){}
};

template <typename T>
struct core_cvs<T*> {
    void (*core_cvs_before_write)(var* v) = cvs_before_write;
    static void core_cvs_after_write(var* v, string delimiter, void* address) {
        T* value;
        PIN_SafeCopy(&value, address, sizeof(value));
        if (invalid_ptr(value))
            return;

        size_t block_size = sizeof(T);
        size_t size = dma_tbl_find(value);
        // FIXME: if not found, the variable is on stack, this is a hack to support common variable on stack, but it does not support array on stack
        if (!size)
            size = block_size;

        cvs<T*> s;
        string vname = v->name;
        for (size_t i = 0; i < size / block_size; i++) {
            if (i != 0)
                v->name = "(" + vname + "+" + std::to_string(i) + ")";
            s.cvs_after_write(v, delimiter, &value);
            value++;
        }
        v->name = vname;
        if (size / block_size > 1)
            cout << "[ARRAY] " << vname << " has " << size / block_size << " elements" << endl;
    }
};

template <typename T>
var* var_construct(void* addr, var* father, string name = DEFAULT_NAME) {
    var* v = new var;
    v->name = name;
    v->address = (void*)addr;
    v->invalid = false;
    v->children.clear();
    v->father.insert(father);
    // v->type = type;
    log<T> l;
    v->log_read = l.log_read;
    v->log_before_write = l.log_before_write;
    v->log_after_write = l.log_after_write;
    core_cvs<T> cc;
    v->cvs_before_write = cc.core_cvs_before_write;
    v->cvs_after_write = cc.core_cvs_after_write;
    return v;
}

// support multi-pointers
template <typename T>
struct cvs<T*> {
    static void cvs_after_write(var* v, string delimiter, void* address) {
        T* value;
        PIN_SafeCopy(&value, address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        var* T_var;
        if (v->name.back() != ')')
            T_var = var_construct<T>(value, v, "*(" + v->name + ")");
        else
            T_var = var_construct<T>(value, v, "*" + v->name);
        i = var_set.find(T_var);
        if (i == var_set.end()) {
            var_set.insert(T_var);
            v->children.push_back(T_var);
            // FIXME: *value could be tricky to get right
            void* ptr;
            PIN_SafeCopy(&ptr, value, sizeof(ptr));
            if (valid_ptr(ptr)) {
                var to_search;
                to_search.address = ptr;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    T_var->cvs_after_write(T_var, DEFAULT_DELIMITER, T_var->address);
                } else {
                    (*it)->father.insert(T_var);
                }
            }
        } else {
            delete T_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

// CVS INIT
struct _compare_function {
    bool operator()(const var* v1, const var* v2) const {
        return v1->name < v2->name;
    }
};

void mmu_parser() {
    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd < 0) exit(-1);
    char buf[512];
    memset(buf, 0, sizeof(buf));
    std::stringstream ss;
    while (read(fd, buf, 500)) {
        ss << buf;
        memset(buf, 0, sizeof(buf));
    }

    string s;
    while (getline(ss, s)) {
        if (!base_address/* && strstr(s.c_str(), app_name.c_str())*/)
            sscanf(s.c_str(), "%lx", &base_address);
        if (!stack_start && strstr(s.c_str(), "[stack]"))
            sscanf(s.c_str(), "%lx-%lx", &stack_start, &stack_start);
    }
    close(fd);
    if (!(base_address && stack_start)) exit(-1);
}

void cvs_init(string app_name) {
    dma_tbl_init();
    set<var*, _compare_function> _vars;
    // Initialize var set
    cout << "Initializing critical variable set\n";

    // FIXME: Currently not support automatically seeking addresses of global struct value
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!! We need a root father here rather than NULL
    var* root = new var;
    root->address = NULL;
    // list test
    // _vars.insert(var_construct<template_list<list>*>(0, root, "l"));
    // _vars.insert(var_construct<template_list<list>*>(0, root, "l2"));
    _vars.insert(var_construct<template_list<int>*>(0, root, "l"));
    _vars.insert(var_construct<template_list<int>*>(0, root, "l2"));
    // array test
    _vars.insert(var_construct<char*>(0, root, "str"));
    // multi-pointer test
    _vars.insert(var_construct<int**>(0, root, "a"));
    // multithreading test
    _vars.insert(var_construct<int>(0, root, "global_int"));
    // vector test
    _vars.insert(var_construct<mock_vector<int>*>(0, root, "v"));
    // NF arp_response test
    _vars.insert(var_construct<state_info*>(0, root, "_state_info"));
    // vftable
    _vars.insert(var_construct<obj*>(0, root, "vobj"));
    // l2fwd
    _vars.insert(var_construct<uint16_t>(0, root, "nb_rxd"));
    _vars.insert(var_construct<uint16_t>(0, root, "nb_txd"));
    _vars.insert(var_construct<l2fwd_port_statistics*>(0, root, "port_statistics"));
    delete root;

    // get base address and the start address of stack
    mmu_parser();
    printf("Program base address: 0x%lx\n", base_address);
    printf("Program stack starting address: 0x%lx\n", stack_start);

    // find global variable by name
    int fd = open(app_name.c_str(), O_RDONLY);
    if (fd < 0) {
        cout << app_name << " not found\n";
        exit(-1);
    }

    struct stat st;
    void *data;
    Elf64_Ehdr *ehdr;
    Elf64_Shdr *shdr;
    Elf64_Sym *symtab;
    char *strtab;

    // Get file information
    fstat(fd, &st);

    // Map the ELF file into memory
    data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        cout << "mmap failed\n";
        close(fd);
        exit(-1);
    }

    // Pointers to ELF header and section header table
    ehdr = (Elf64_Ehdr *) data;
    shdr = (Elf64_Shdr *) ((uint64_t)data + ehdr->e_shoff);

    // Find the symbol table section
    Elf64_Shdr *symtab_hdr = NULL;
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            symtab_hdr = &shdr[i];
            break;
        }
    }

    if (symtab_hdr == NULL) {
        cout << "Symbol table section not found\n";
        munmap(data, st.st_size);
        close(fd);
        exit(-1);
    }

    // Get symbol table and string table pointers
    symtab = (Elf64_Sym *) ((uint64_t)data + symtab_hdr->sh_offset);
    strtab = (char *) ((uint64_t)data + shdr[symtab_hdr->sh_link].sh_offset);

    // Iterate through symbols and print them
    for (size_t i = 0; i < symtab_hdr->sh_size / sizeof(Elf64_Sym); i++) {
        string vname(&strtab[symtab[i].st_name]);
        var to_search;
        to_search.name = vname;
        auto it = _vars.find(&to_search);
        if (it != _vars.end()) {
            if ((*it)->name == "port_statistics") {
                void* fake_ptr = (void*)(base_address + symtab[i].st_value);
                (*it)->cvs_after_write(*it, "->", &fake_ptr);
                cout << "Found variable " << vname << "[32] at address " << (void*)(base_address + symtab[i].st_value) << endl;
                continue;
            }
            (*it)->address = (void*)(base_address + symtab[i].st_value);
            var_set.insert(*it);
            cout << "Found variable " << vname << " at address " << (*it)->address << endl;
        }
    }

    // Clean up
    for (auto j : _vars) {
        if (j->address == 0) delete j;
    }
    _vars.clear();
    munmap(data, st.st_size);
    close(fd);

    cout << "Critical variable set initialized\n";
    fflush(stdout);
}
}

