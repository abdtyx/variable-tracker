#ifndef __TYPEHELPER_HH
#define __TYPEHELPER_HH

namespace vt {

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

template <typename T>
struct mock_vector {
    T* _M_start;
    T* _M_finish;
    T* _M_end_of_storage;
};

}
#endif