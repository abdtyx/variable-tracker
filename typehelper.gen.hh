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

struct var {
    string name;                              // 变量名
    void* address;                            // 变量地址
    bool invalid;                             // 暂时没用
    int type;                                 // 变量类型，0为普通变量，1为指针变量
    uint64_t reference_counter;               // reference counter是一个用来记录有多少个指针指向了这一块地址的计数器
    vector<var*> children;                    // children存储了这个指针指向的所有变量的地址。
    void (*log_read)(var* v);             // Read的log函数
    void (*log_before_write)(var* v);     // BeforeWrite的log函数
    void (*log_after_write)(var* v);      // AfterWrite的log函数
    void (*set_before_write)(var* v);     // 在BeforeWrite中操作set的函数
    void (*set_after_write)(var* v);      // 在AfterWrite中操作set的函数
};

struct var_set_cmp {
    bool operator()(const var* v1, const var* v2) const {
        return v1->address < v2->address;
    }
};

void print_var(var* v, string space = "") {
    cout << space << "var {name: " << v->name << ", address: " << v->address << ", invalid: " << v->invalid << ", type: " << v->type << ", reference_counter: " << v->reference_counter << "}" << endl;
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
    value >>= 32;
    // cout << "[value]: " << value << endl;
    return (value == 0x5555 || value == 0x7fff);
}

bool invalid_ptr(void* addr) {
    return !valid_ptr(addr);
}

set<var*, var_set_cmp> var_set;
map<void*, set<void*> > ptr_addr;

void ptr_addr_add(void* k, void* v) {
    // cout << "Ins: " << k << " : " << v << endl;
    if (ptr_addr.find(k) == ptr_addr.end()) {
        // set<void*> temp;
        // temp.clear();
        ptr_addr[k] = {};
    }
    ptr_addr[k].insert(v);
}

void ptr_addr_remove(void* k, void* v) {
    // cout << "Rm: " << k << " : " << v << endl;
    if (k == NULL) {
        return;
    }
    auto it = ptr_addr.find(k);
    if (it == ptr_addr.end())
        return;
    it->second.erase(v);
    if (it->second.size() == 0)
        ptr_addr.erase(it);
}

// list

struct list {
    int a;
    double* b;
    struct list* next;
};

var* list_var_construct(void* addr, uint64_t type, string name = DEFAULT_NAME);
void list_log_read(var* v);
void list_log_before_write(var* v);
void list_log_after_write(var* v);
void list_set_before_write(var* v);
void list_set_after_write(var* v);
var* double_var_construct(void* addr, uint64_t type, string name = DEFAULT_NAME);
void double_log_read(var* v);
void double_log_before_write(var* v);
void double_log_after_write(var* v);
void double_set_before_write(var* v);
void double_set_after_write(var* v);
var* int_var_construct(void* addr, uint64_t type, string name = DEFAULT_NAME);
void int_log_read(var* v);
void int_log_before_write(var* v);
void int_log_after_write(var* v);
void int_set_before_write(var* v);
void int_set_after_write(var* v);

void set_before_write(var* v);

void set_before_write(var* v) {
    if (v->type == TYPE_POINTER) {
        void* value;
        PIN_SafeCopy(&value, v->address, sizeof(value));
        ptr_addr_remove(value, v->address);
    }
    for (auto child : v->children) {
        child->reference_counter--;
        if (child->reference_counter == 0) {
            child->set_before_write(child);
            // cout << var_set.size() << endl;
            var_set.erase(child);
            // cout << var_set.size() << endl;
            free(child);
        }
    }
    v->children.clear();
}

var* list_var_construct(void* addr, uint64_t type, string name) {
    var* list_var = (var*)calloc(1, sizeof(var));
    list_var->name = name;
    list_var->address = (void*)addr;
    list_var->invalid = false;
    list_var->children.clear();
    list_var->reference_counter = 1;
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

    ptr_addr_add(value, v->address);

    set<var *, var_set_cmp>::iterator i;

    // list* next
    var* list_var = list_var_construct(&(value->next), TYPE_POINTER, "next");
    i = var_set.find(list_var);
    if (i == var_set.end()) {
        var_set.insert(list_var);
        v->children.push_back(list_var);
    } else {
        (*i)->reference_counter++;
        v->children.push_back(*i);
    }
    if (valid_ptr(value->next)) {
        var to_search;
        to_search.address = value->next;
        auto it = var_set.find(&to_search);
        if (it == var_set.end()) {
            list_var->set_after_write(list_var);
        } else {
            (*it)->reference_counter++;
        }
    }

    // int a
    var* int_var = int_var_construct(&(value->a), TYPE_VAR, "a");
    i = var_set.find(int_var);
    if (i == var_set.end()) {
        var_set.insert(int_var);
        v->children.push_back(int_var);
    } else {
        (*i)->reference_counter++;
        v->children.push_back(*i);
    }

    // double b
    var* double_var = double_var_construct(&(value->b), TYPE_POINTER, "b");
    i = var_set.find(double_var);
    if (i == var_set.end()) {
        var_set.insert(double_var);
        v->children.push_back(double_var);
    } else {
        (*i)->reference_counter++;
        v->children.push_back(*i);
    }
    if (valid_ptr(value->b)) {
        var to_search;
        to_search.address = value->b;
        auto it = var_set.find(&to_search);
        if (it == var_set.end()) {
            double_var->set_after_write(double_var);
        } else {
            (*it)->reference_counter++;
        }
    }
    // print_var(v);
}

// double

var* double_var_construct(void* addr, uint64_t type, string name) {
    var* double_var = (var*)calloc(1, sizeof(var));
    double_var->name = name;
    double_var->address = (void*)addr;
    double_var->invalid = false;
    double_var->children.clear();
    double_var->reference_counter = 1;
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

    ptr_addr_add(value, v->address);

    set<var *, var_set_cmp>::iterator i;

    var* double_var = double_var_construct(value, TYPE_VAR);
    i = var_set.find(double_var);
    if (i == var_set.end()) {
        var_set.insert(double_var);
        v->children.push_back(double_var);
    } else {
        (*i)->reference_counter++;
        v->children.push_back(*i);
    }
}

// int

var* int_var_construct(void* addr, uint64_t type, string name) {
    var* int_var = (var*)calloc(1, sizeof(var));
    int_var->name = name;
    int_var->address = (void*)addr;
    int_var->invalid = false;
    int_var->children.clear();
    int_var->reference_counter = 1;
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

    ptr_addr_add(value, v->address);

    set<var *, var_set_cmp>::iterator i;

    var* int_var = int_var_construct(value, TYPE_VAR);
    i = var_set.find(int_var);
    if (i == var_set.end()) {
        var_set.insert(int_var);
        v->children.push_back(int_var);
    } else {
        (*i)->reference_counter++;
        v->children.push_back(*i);
    }
}
