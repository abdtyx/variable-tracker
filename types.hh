#ifndef __TYPES_HH
#define __TYPES_HH

//////////////////////////////////////////////////////
////////////////// type declaration //////////////////
//////////////////////////////////////////////////////

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

template <typename T>
class template_list {
public:
    T a;
    double* b;
    template_list* next;
};

// STL vector
template <typename T>
struct mock_vector {
    T* _M_start;
    T* _M_finish;
    T* _M_end_of_storage;
};

// A 24-byte object, starting with a pointer that points to the vtable
struct obj {
    uint64_t a;
    static uint64_t num;
    const uint64_t buf = 255;
    void foo() {}
    static const uint64_t sc;
    virtual void bar() {}
};

// NF arp_response
struct state_info {
    void* pktmbuf_pool;
    uint16_t nf_destination;
    uint32_t* source_ips;
    int print_flag;
};

// l2fwd_port_statistics
/* Per-port statistics struct */
struct l2fwd_port_statistics {
	uint64_t tx;
	uint64_t rx;
	uint64_t dropped;
} __attribute__((__aligned__(64)));

#endif