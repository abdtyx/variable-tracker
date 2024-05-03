#ifndef __CVSTYPES_HH
#define __CVSTYPES_HH

#include "vt.hh"
#include "types.hh"

namespace vt {

#define CONSTRUCT_VAR(type, field) type* fake_name ## field = &(value->field); cvs<type*>::cvs_after_write(v, delimiter + #field ".", &fake_name ## field);

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
        CONSTRUCT_VAR(list*, next);

        // int a
        CONSTRUCT_VAR(int, a);

        // double* b
        CONSTRUCT_VAR(double*, b);
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
        CONSTRUCT_VAR(template_list<T>*, next);

        // T a
        CONSTRUCT_VAR(T, a)

        // double* b
        CONSTRUCT_VAR(double*, b);
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