#include "pin.H"
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <set>
#include <map>
#include <string>

#include <boost/type_index.hpp>

// #define TYPE_VAR 0
// #define TYPE_POINTER 1
#define DEFAULT_NAME "__DEFAULT_NAME__"

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
    // int type;                                 // 变量类型，0为普通变量，1为指针变量
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
struct mockset {
    static void set_before_write(var* v){}
    static void set_after_write(var* v){}
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
    mockset<T> s;
    v->set_before_write = s.set_before_write;
    v->set_after_write = s.set_after_write;
    return v;
}

template<>
struct mockset<list*> {
    void (*set_before_write)(var* v) = general_set_before_write;

    static void set_after_write(var* v) {
        // print_var(v);
        list* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        // list* next
        var* list_var = var_construct<list*>(&(value->next), v, "next");
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
            delete list_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // int a
        var* int_var = var_construct<int>(&(value->a), v, "a");
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
        var* double_var = var_construct<double*>(&(value->b), v, "b");
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
            delete double_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
        // print_var(v);
    }
};

template<>
struct mockset<double*> {
    void (*set_before_write)(var* v) = general_set_before_write;

    static void set_after_write(var* v) {
        double* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        var* double_var = var_construct<double>(value, v);
        i = var_set.find(double_var);
        if (i == var_set.end()) {
            var_set.insert(double_var);
            v->children.push_back(double_var);
        } else {
            delete double_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

template <>
struct mockset<int*> {
    void (*set_before_write)(var* v) = general_set_before_write;

    static void set_after_write(var* v) {
        int* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        var* int_var = var_construct<int>(value, v);
        i = var_set.find(int_var);
        if (i == var_set.end()) {
            var_set.insert(int_var);
            v->children.push_back(int_var);
        } else {
            delete int_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

template <typename T>
class template_list {
public:
    T a;
    double* b;
    template_list* next;
};

template <typename T>
struct mockset<template_list<T>*> {
    void (*set_before_write)(var* v) = general_set_before_write;

    static void set_after_write(var* v) {
        template_list<T>* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;

        // template_list* next
        var* template_list_var = var_construct<template_list<T>*>(&(value->next), v, "next");
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
                    template_list_var->set_after_write(template_list_var);
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
        var* T_var = var_construct<T>(&(value->a), v, "a");
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
        var* double_var = var_construct<double*>(&(value->b), v, "b");
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
            delete double_var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
        // print_var(v);
    }
};

void cvs_init() {
    // Initialize var set
    cout << "Initializing critical variable set\n";
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!! We need a root father here rather than NULL
    var* root = new var;
    root->address = NULL;
    // var* list_var_l = var_construct<template_list<int>>((void*)0x555555558158, TYPE_POINTER, root, "l");
    // var_set.insert(list_var_l);

    // var* list_var_l2 = var_construct<template_list<int>>((void*)0x555555558160, TYPE_POINTER, root, "l2");
    // var_set.insert(list_var_l2);

    var* list_var_l = var_construct<list*>((void*)0x555555558158, root, "l");
    var_set.insert(list_var_l);

    // var* list_var_l2 = var_construct<list>((void*)0x555555558160, TYPE_VAR, root, "l2");
    // var_set.insert(list_var_l2);

    // var* int_var = int_var_construct((void*)0x555555558158, TYPE_POINTER, "globall");
    // var_set.insert(int_var);
    // var* list_var = list_var_construct((void*)0x555555558160, TYPE_POINTER, "l");
    // var_set.insert(list_var);

    // var* template_list_l = template_list<int>::template_list_var_construct((void*)0x555555558158, TYPE_POINTER, root, "l");
    // var_set.insert(template_list_l);
    // var* template_list_l2 = template_list<int>::template_list_var_construct((void*)0x555555558160, TYPE_POINTER, root, "l2");
    // var_set.insert(template_list_l2);

    cout << "Critical variable set initialized\n";
}
}

