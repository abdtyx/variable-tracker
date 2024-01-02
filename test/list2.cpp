#include <iostream>

using namespace std;

struct list {
    int a;
    double* b;
    struct list* next;
};

list* l;
list* l2;

int main() {
    cout << "Alloc a list" << endl;
    l = (list*)malloc(sizeof(list));

    cout << "Another pointer l2 refers to the list" << endl;
    l2 = l;

    cout << "Alloc another list to l without free the previous one" << endl;
    l = (list*)malloc(sizeof(list));

    cout << "Alloc another list to l2 without free the previous one" << endl;
    l2 = (list*)malloc(sizeof(list));

    cout << "Set l2 = l without free the previously allocated *l2" << endl;
    l2 = l;
}