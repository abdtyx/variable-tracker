#include <iostream>

using namespace std;

struct list {
    int a;
    double* b;
    struct list* next;
};

list *l;

int main() {
    // cout << &l << endl;
    double abc = 2.0;
    l = (list*)calloc(1, sizeof(list));
    cout << l << endl;
    l->a = 1;
    l->b = &abc;
    int aa = l->a;
    double abcabc = *l->b;
    abc = *l->b;
    l->next = (list*)calloc(1, sizeof(list));
    cout << l->next << endl;
    free(l->next);
    l->next = NULL;
    free(l);
    l = NULL;
}