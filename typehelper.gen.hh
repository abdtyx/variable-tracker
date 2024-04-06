#include "pin.H"
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <boost/type_index.hpp>

#define DEFAULT_NAME "__DEFAULT_NAME__"
#define DEFAULT_DELIMITER "->"

using std::cout, std::endl, std::vector, std::set, std::map, std::string;

namespace vt {
template <typename T>
struct compare_function {
    bool operator()(const T v1, const T v2) const {
        return v1->address < v2->address;
    }
};

struct var {
    string name;                              // 变量名
    void* address;                            // 变量地址
    bool invalid;                             // 暂时没用
    set<var*, compare_function<var*> > father;// who points at this var
    vector<var*> children;                    // children存储了这个指针指向的所有变量的地址。
    void (*log_read)(var* v);             // Read的log函数
    void (*log_before_write)(var* v);     // BeforeWrite的log函数
    void (*log_after_write)(var* v);      // AfterWrite的log函数
    void (*set_before_write)(var* v);     // 在BeforeWrite中操作set的函数
    void (*set_after_write)(var* v, string delimiter);      // 在AfterWrite中操作set的函数

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

bool valid_ptr(void* addr) {
    // cout << "[validate]: " << addr << endl;
    uint64_t value = (uint64_t)addr;
    value >>= 40;
    // cout << "[value]: " << value << endl;
    return (value == 0x55 || value == 0x7f);
}

bool invalid_ptr(void* addr) {
    return !valid_ptr(addr);
}

set<var*, compare_function<var*> > var_set;

// spin-lock
#include <stdatomic.h>
#include <sched.h>

atomic_flag lock = ATOMIC_FLAG_INIT;

void lock_acquire() {
    while (atomic_flag_test_and_set(&lock))
        sched_yield();
}

void lock_release() {
    atomic_flag_clear(&lock);
}

// list

// struct list {
//     int a;
//     double* b;
//     struct list* next;
// };
class list {
public:
    int a;
    double* b;
    list* next;
};

void general_set_before_write(var* v) {
    for (auto child : v->children) {
        child->father.erase(v);
        if (child->father.size() == 0) {
            child->set_before_write(child);
            // cout << var_set.size() << endl;
            var_set.erase(child);
            // cout << var_set.size() << endl;
            delete child;
        }
    }
    v->children.clear();
}

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

template <typename T>
struct log<T*> {
    static void log_read(var* v) {
        T* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] " << boost::typeindex::type_id<T*>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
    static void log_before_write(var* v) {
        T* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] " << boost::typeindex::type_id<T*>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
    static void log_after_write(var* v) {
        T* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] " << boost::typeindex::type_id<T*>().pretty_name() << ' ' << v->name << ' ' << value << endl;
    }
};

template <typename T>
struct cvs {
    static void set_before_write(var* v){}
    static void set_after_write(var* v, string delimiter){}
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
    cvs<T> s;
    v->set_before_write = s.set_before_write;
    v->set_after_write = s.set_after_write;
    return v;
}

// support multi-pointers
template <typename T>
struct cvs<T*> {
    void (*set_before_write)(var* v) = general_set_before_write;

    static void set_after_write(var* v, string delimiter) {
        T* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        var* T_var = var_construct<T>(value, v, "*(" + v->name + ")");
        i = var_set.find(T_var);
        if (i == var_set.end()) {
            var_set.insert(T_var);
            v->children.push_back(T_var);
        } else {
            delete T_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

template<>
struct cvs<list*> {
    void (*set_before_write)(var* v) = general_set_before_write;

    static void set_after_write(var* v, string delimiter) {
        // print_var(v);
        list* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        // list* next
        var* list_var = var_construct<list*>(&(value->next), v, v->name + delimiter + "next");
        i = var_set.find(list_var);
        if (i == var_set.end()) {
            var_set.insert(list_var);
            v->children.push_back(list_var);
            // if valid ptr
            if (valid_ptr(value->next)) {
                var to_search;
                to_search.address = value->next;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    list_var->set_after_write(list_var, DEFAULT_DELIMITER);
                } else {
                    (*it)->father.insert(list_var);
                }
            }
        } else {
            delete list_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // int a
        var* int_var = var_construct<int>(&(value->a), v, v->name + delimiter + "a");
        i = var_set.find(int_var);
        if (i == var_set.end()) {
            var_set.insert(int_var);
            v->children.push_back(int_var);
        } else {
            delete int_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // double* b
        var* double_var = var_construct<double*>(&(value->b), v, v->name + delimiter + "b");
        i = var_set.find(double_var);
        if (i == var_set.end()) {
            var_set.insert(double_var);
            v->children.push_back(double_var);
            if (valid_ptr(value->b)) {
                var to_search;
                to_search.address = value->b;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    double_var->set_after_write(double_var, DEFAULT_DELIMITER);
                } else {
                    (*it)->father.insert(double_var);
                }
            }
        } else {
            delete double_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
        // print_var(v);
    }
};

// template<>
// struct cvs<double*> {
//     void (*set_before_write)(var* v) = general_set_before_write;

//     static void set_after_write(var* v, string delimiter) {
//         double* value;
//         PIN_SafeCopy(&value, v->address, sizeof(value));
//         if (invalid_ptr(value))
//             return;

//         set<var*>::iterator i;

//         var* double_var = var_construct<double>(value, v);
//         i = var_set.find(double_var);
//         if (i == var_set.end()) {
//             var_set.insert(double_var);
//             v->children.push_back(double_var);
//         } else {
//             delete double_var;
//             (*i)->father.insert(v);
//             v->children.push_back(*i);
//         }
//     }
// };

// template <>
// struct cvs<int*> {
//     void (*set_before_write)(var* v) = general_set_before_write;

//     static void set_after_write(var* v, string delimiter) {
//         int* value;
//         PIN_SafeCopy(&value, v->address, sizeof(value));
//         if (invalid_ptr(value))
//             return;

//         set<var*>::iterator i;

//         var* int_var = var_construct<int>(value, v);
//         i = var_set.find(int_var);
//         if (i == var_set.end()) {
//             var_set.insert(int_var);
//             v->children.push_back(int_var);
//         } else {
//             delete int_var;
//             (*i)->father.insert(v);
//             v->children.push_back(*i);
//         }
//     }
// };

template <typename T>
class template_list {
public:
    T a;
    double* b;
    template_list* next;
};

template <typename T>
struct cvs<template_list<T>*> {
    void (*set_before_write)(var* v) = general_set_before_write;

    static void set_after_write(var* v, string delimiter) {
        template_list<T>* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        // template_list* next
        var* template_list_var = var_construct<template_list<T>*>(&(value->next), v, v->name + delimiter + "next");
        i = var_set.find(template_list_var);
        if (i == var_set.end()) {
            var_set.insert(template_list_var);
            v->children.push_back(template_list_var);
            // if valid ptr
            if (valid_ptr(value->next)) {
                var to_search;
                to_search.address = value->next;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    template_list_var->set_after_write(template_list_var, DEFAULT_DELIMITER);
                } else {
                    (*it)->father.insert(template_list_var);
                }
            }
        } else {
            delete template_list_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // T a
        var* T_var = var_construct<T>(&(value->a), v, v->name + delimiter + "a");
        i = var_set.find(T_var);
        if (i == var_set.end()) {
            var_set.insert(T_var);
            v->children.push_back(T_var);
        } else {
            delete T_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // double* b
        var* double_var = var_construct<double*>(&(value->b), v, v->name + delimiter + "b");
        i = var_set.find(double_var);
        if (i == var_set.end()) {
            var_set.insert(double_var);
            v->children.push_back(double_var);
            if (valid_ptr(value->b)) {
                var to_search;
                to_search.address = value->b;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    double_var->set_after_write(double_var, DEFAULT_DELIMITER);
                } else {
                    (*it)->father.insert(double_var);
                }
            }
        } else {
            delete double_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
        // print_var(v);
    }
};

// CVS INIT
struct _compare_function {
    bool operator()(const var* v1, const var* v2) const {
        return v1->name < v2->name;
    }
};

void cvs_init(string app_name) {
    set<var*, _compare_function> _vars;
    uint64_t base_address;
    // Initialize var set
    cout << "Initializing critical variable set\n";

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!! We need a root father here rather than NULL
    var* root = new var;
    root->address = NULL;
    _vars.insert(var_construct<list*>(0, root, "l"));
    _vars.insert(var_construct<list*>(0, root, "l2"));
    delete root;

    // get base address
    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd < 0) exit(-1);
    char buf[512];
    memset(buf, 0, sizeof(buf));
    read(fd, buf, 500);
    if (strstr(buf, app_name.c_str())) {
        if (sscanf(buf, "%lx", &base_address) <= 0) {
            close(fd);
            exit(-1);
        }
    } else {
        close(fd);
        exit(-1);
    }
    printf("Program base address: %lx\n", base_address);
    close(fd);

    // find global variable by name
    fd = open(app_name.c_str(), O_RDONLY);
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

