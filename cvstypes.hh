#ifndef __CVSTYPES_HH
#define __CVSTYPES_HH

#include "vt.hh"
#include "types.hh"

namespace vt {

template<>
struct cvs<list*> {
    static void cvs_after_write(var* v, string delimiter, void* address) {
        // print_var(v);
        list* value;
        PIN_SafeCopy(&value, address, sizeof(value));
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
                    list_var->cvs_after_write(list_var, DEFAULT_DELIMITER, list_var->address);
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
                    double_var->cvs_after_write(double_var, DEFAULT_DELIMITER, double_var->address);
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

template <typename T>
struct cvs<template_list<T>*> {
    static void cvs_after_write(var* v, string delimiter, void* address) {
        template_list<T>* value;
        PIN_SafeCopy(&value, address, sizeof(value));
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
                    template_list_var->cvs_after_write(template_list_var, DEFAULT_DELIMITER, template_list_var->address);
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
        // var* T_var = var_construct<T>(&(value->a), v, v->name + delimiter + "a");
        // i = var_set.find(T_var);
        // if (i == var_set.end()) {
        //     var_set.insert(T_var);
        //     v->children.push_back(T_var);
        // } else {
        //     delete T_var;
        //     (*i)->father.insert(v);
        //     v->children.push_back(*i);
        // }
        T* fake_ptr = &(value->a);
        cvs<T*> temp_cvs;
        temp_cvs.cvs_after_write(v, "->a.", &fake_ptr);

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
                    double_var->cvs_after_write(double_var, DEFAULT_DELIMITER, double_var->address);
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

template <typename T>
struct cvs<mock_vector<T>*> {
    static void cvs_after_write(var* v, string delimiter, void* address) {
        mock_vector<T>* value;
        PIN_SafeCopy(&value, address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;
        var* _var;

        // T* _M_start
        _var = var_construct<T*>(&(value->_M_start), v, v->name + delimiter + "_M_start");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
            if (valid_ptr(value->_M_start)) {
                var to_search;
                to_search.address = value->_M_start;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    _var->cvs_after_write(_var, DEFAULT_DELIMITER, _var->address);
                } else {
                    (*it)->father.insert(_var);
                }
            }
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // T* _M_finish
        _var = var_construct<T*>(&(value->_M_finish), v, v->name + delimiter + "_M_finish");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
            if (valid_ptr(value->_M_finish)) {
                var to_search;
                to_search.address = value->_M_finish;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    _var->cvs_after_write(_var, DEFAULT_DELIMITER, _var->address);
                } else {
                    (*it)->father.insert(_var);
                }
            }
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // T* _M_end_of_storage
        _var = var_construct<T*>(&(value->_M_end_of_storage), v, v->name + delimiter + "_M_end_of_storage");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
            if (valid_ptr(value->_M_end_of_storage)) {
                var to_search;
                to_search.address = value->_M_end_of_storage;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    _var->cvs_after_write(_var, DEFAULT_DELIMITER, _var->address);
                } else {
                    (*it)->father.insert(_var);
                }
            }
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

// Code to test a 24-byte object
template <>
struct cvs<obj*> {
    static void cvs_after_write(var* v, string delimiter, void* address) {
        obj* value;
        PIN_SafeCopy(&value, address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;
        var* _var;

        // void*
        _var = var_construct<void*>(value, v, v->name + delimiter + "var1");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // void*
        _var = var_construct<void*>(value + 0x8, v, v->name + delimiter + "var2");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // void*
        _var = var_construct<void*>(value + 0x10, v, v->name + delimiter + "var3");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

template <>
struct cvs<state_info*> {
    static void cvs_after_write(var* v, string delimiter, void* address) {
        state_info* value;
        PIN_SafeCopy(&value, address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;
        var* _var;

        // void* pktmbuf_pool;
        _var = var_construct<void*>(&(value->pktmbuf_pool), v, v->name + delimiter + "pktmbuf_pool");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // uint16_t nf_destination
        _var = var_construct<uint16_t>(&(value->nf_destination), v, v->name + delimiter + "nf_destination");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // uint32_t* source_ips
        _var = var_construct<uint32_t*>(&(value->source_ips), v, v->name + delimiter + "source_ips");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
            if (valid_ptr(value->source_ips)) {
                var to_search;
                to_search.address = value->source_ips;
                auto it = var_set.find(&to_search);
                if (it == var_set.end()) {
                    _var->cvs_after_write(_var, DEFAULT_DELIMITER, _var->address);
                } else {
                    (*it)->father.insert(_var);
                }
            }
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // int print_flag
        _var = var_construct<int>(&(value->print_flag), v, v->name + delimiter + "print_flag");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

template <>
struct cvs<l2fwd_port_statistics*> {
    static void cvs_after_write(var* v, string delimiter, void* address) {
        l2fwd_port_statistics* value;
        PIN_SafeCopy(&value, address, sizeof(value));
        if (invalid_ptr(value))
            return;

        set<var*>::iterator i;
        var* _var;

        // uint64_t tx
        _var = var_construct<uint64_t>(&(value->tx), v, v->name + delimiter + "tx");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // uint64_t rx
        _var = var_construct<uint64_t>(&(value->rx), v, v->name + delimiter + "rx");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }

        // uint64_t dropped
        _var = var_construct<uint64_t>(&(value->dropped), v, v->name + delimiter + "dropped");
        i = var_set.find(_var);
        if (i == var_set.end()) {
            var_set.insert(_var);
            v->children.push_back(_var);
        } else {
            delete _var;
            (*i)->father.insert(v);
            v->children.push_back(*i);
        }
    }
};

}

#endif