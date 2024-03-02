#include "pin.H"
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <set>
#include <map>
#include <string>

#define TYPE_VAR 0
#define TYPE_POINTER 1
#define DEFAULT_NAME "__DEFAULT_NAME__"

using std::cout, std::endl, std::vector, std::set, std::map, std::string;

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
    int type;                                 // 变量类型，0为普通变量，1为指针变量
    set<var*, compare_function<var*> > father;// who points at this var
    vector<var*> children;                    // children存储了这个指针指向的所有变量的地址。
    void (*log_read)(var* v);             // Read的log函数
    void (*log_before_write)(var* v);     // BeforeWrite的log函数
    void (*log_after_write)(var* v);      // AfterWrite的log函数
    void (*set_before_write)(var* v);     // 在BeforeWrite中操作set的函数
    void (*set_after_write)(var* v);      // 在AfterWrite中操作set的函数

    // compare function
    bool operator()(const var* v1, const var* v2) const {
        return v1->address < v2->address;
    }
};

void print_var(var* v, string space = "") {
    cout << space << "var {name: " << v->name << ", address: " << v->address << ", invalid: " << v->invalid << ", type: " << v->type << ", reference_counter: " << v->father.size() << "}" << endl;
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

struct list {
    int a;
    double* b;
    struct list* next;
};

var* list_var_construct(void* addr, uint64_t type, var* father, string name = DEFAULT_NAME);
void list_log_read(var* v);
void list_log_before_write(var* v);
void list_log_after_write(var* v);
void list_set_before_write(var* v);
void list_set_after_write(var* v);
var* double_var_construct(void* addr, uint64_t type, var* father, string name = DEFAULT_NAME);
void double_log_read(var* v);
void double_log_before_write(var* v);
void double_log_after_write(var* v);
void double_set_before_write(var* v);
void double_set_after_write(var* v);
var* int_var_construct(void* addr, uint64_t type, var* father, string name = DEFAULT_NAME);
void int_log_read(var* v);
void int_log_before_write(var* v);
void int_log_after_write(var* v);
void int_set_before_write(var* v);
void int_set_after_write(var* v);

void set_before_write(var* v);

void set_before_write(var* v) {
    for (auto child : v->children) {
        child->father.erase(v);
        if (child->father.size() == 0) {
            child->set_before_write(child);
            // cout << var_set.size() << endl;
            var_set.erase(child);
            // cout << var_set.size() << endl;
            free(child);
        }
    }
    v->children.clear();
}

var* list_var_construct(void* addr, uint64_t type, var* father, string name) {
    var* list_var = new var;
    list_var->name = name;
    list_var->address = (void*)addr;
    list_var->invalid = false;
    list_var->children.clear();
    list_var->father.insert(father);
    list_var->type = type;
    list_var->log_read = list_log_read;
    list_var->log_before_write = list_log_before_write;
    list_var->log_after_write = list_log_after_write;
    list_var->set_before_write = set_before_write;
    list_var->set_after_write = list_set_after_write;
    return list_var;
}

void list_log_read(var* v) {
    list* value;
    PIN_SafeCopy(&value, v->address, sizeof(value));
    cout << "[READ] list* " << v->name << ' ' << value << endl;
}

void list_log_before_write(var* v) {
    list* value;
    PIN_SafeCopy(&value, v->address, sizeof(value));
    cout << "[BEFORE WRITE] list* " << v->name << ' ' << value << endl;
}

void list_log_after_write(var* v) {
    list* value;
    PIN_SafeCopy(&value, v->address, sizeof(value));
    cout << "[AFTER WRITE] list* " << v->name << ' ' << value << endl;
}

void list_set_before_write(var* v) {
}

void list_set_after_write(var* v) {
    // print_var(v);
    list* value;
    PIN_SafeCopy(&value, v->address, sizeof(value));
    if (invalid_ptr(value))
        return;

    set<var*>::iterator i;

    // list* next
    var* list_var = list_var_construct(&(value->next), TYPE_POINTER, v, "next");
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
                list_var->set_after_write(list_var);
            } else {
                (*it)->father.insert(list_var);
            }
        }
    } else {
        free(list_var);
        (*i)->father.insert(v);
        v->children.push_back(*i);
    }

    // int a
    var* int_var = int_var_construct(&(value->a), TYPE_VAR, v, "a");
    i = var_set.find(int_var);
    if (i == var_set.end()) {
        var_set.insert(int_var);
        v->children.push_back(int_var);
    } else {
        free(int_var);
        (*i)->father.insert(v);
        v->children.push_back(*i);
    }

    // double* b
    var* double_var = double_var_construct(&(value->b), TYPE_POINTER, v, "b");
    i = var_set.find(double_var);
    if (i == var_set.end()) {
        var_set.insert(double_var);
        v->children.push_back(double_var);
        if (valid_ptr(value->b)) {
            var to_search;
            to_search.address = value->b;
            auto it = var_set.find(&to_search);
            if (it == var_set.end()) {
                double_var->set_after_write(double_var);
            } else {
                (*it)->father.insert(double_var);
            }
        }
    } else {
        free(double_var);
        (*i)->father.insert(v);
        v->children.push_back(*i);
    }
    // print_var(v);
}

// double

var* double_var_construct(void* addr, uint64_t type, var* father, string name) {
    var* double_var = new var;
    double_var->name = name;
    double_var->address = (void*)addr;
    double_var->invalid = false;
    double_var->children.clear();
    double_var->father.insert(father);
    double_var->type = type;
    double_var->log_read = double_log_read;
    double_var->log_before_write = double_log_before_write;
    double_var->log_after_write = double_log_after_write;
    double_var->set_before_write = set_before_write;
    double_var->set_after_write = double_set_after_write;
    return double_var;
}

void double_log_read(var* v) {
    if (v->type == TYPE_VAR) {
        double value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] double " << v->name << ' ' << value << endl;
    }
    else if (v->type == TYPE_POINTER) {
        double* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] double* " << v->name << ' ' << value << endl;
    }
}

void double_log_before_write(var* v) {
    if (v->type == TYPE_VAR) {
        double value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] double " << v->name << ' ' << value << endl;
    }
    else if (v->type == TYPE_POINTER) {
        double* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] double* " << v->name << ' ' << value << endl;
    }
}

void double_log_after_write(var* v) {
    if (v->type == TYPE_VAR) {
        double value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] double " << v->name << ' ' << value << endl;
    }
    else if (v->type == TYPE_POINTER) {
        double* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] double* " << v->name << ' ' << value << endl;
    }
}

void double_set_before_write(var* v) {
}

void double_set_after_write(var* v) {
    double* value;
    PIN_SafeCopy(&value, v->address, sizeof(value));
    if (invalid_ptr(value))
        return;

    set<var*>::iterator i;

    var* double_var = double_var_construct(value, TYPE_VAR, v);
    i = var_set.find(double_var);
    if (i == var_set.end()) {
        var_set.insert(double_var);
        v->children.push_back(double_var);
    } else {
        free(double_var);
        (*i)->father.insert(v);
        v->children.push_back(*i);
    }
}

// int

var* int_var_construct(void* addr, uint64_t type, var* father, string name) {
    var* int_var = new var;
    int_var->name = name;
    int_var->address = (void*)addr;
    int_var->invalid = false;
    int_var->children.clear();
    int_var->father.insert(father);
    int_var->type = type;
    int_var->log_read = int_log_read;
    int_var->log_before_write = int_log_before_write;
    int_var->log_after_write = int_log_after_write;
    int_var->set_before_write = set_before_write;
    int_var->set_after_write = int_set_after_write;
    return int_var;
}

void int_log_read(var* v) {
    if (v->type == TYPE_VAR) {
        int value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] int " << v->name << ' ' << value << endl;
    }
    else if (v->type == TYPE_POINTER) {
        int* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[READ] int* " << v->name << ' ' << value << endl;
    }
}

void int_log_before_write(var* v) {
    if (v->type == TYPE_VAR) {
        int value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] int " << v->name << ' ' << value << endl;
    }
    else if (v->type == TYPE_POINTER) {
        int* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[BEFORE WRITE] int* " << v->name << ' ' << value << endl;
    }
}

void int_log_after_write(var* v) {
    if (v->type == TYPE_VAR) {
        int value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] int " << v->name << ' ' << value << endl;
    }
    else if (v->type == TYPE_POINTER) {
        int* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        cout << "[AFTER WRITE] int* " << v->name << ' ' << value << endl;
    }
}

void int_set_before_write(var* v) {
}

void int_set_after_write(var* v) {
    int* value;
    PIN_SafeCopy(&value, v->address, sizeof(value));
    if (invalid_ptr(value))
        return;

    set<var*>::iterator i;

    var* int_var = int_var_construct(value, TYPE_VAR, v);
    i = var_set.find(int_var);
    if (i == var_set.end()) {
        var_set.insert(int_var);
        v->children.push_back(int_var);
    } else {
        free(int_var);
        (*i)->father.insert(v);
        v->children.push_back(*i);
    }
}
