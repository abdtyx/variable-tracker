#include <pthread.h>
#include <bits/stdc++.h>

using namespace std;

pthread_t threads[10];

int global_int = 0;

void* func(void* arg) {
    uint64_t i = (uint64_t)arg;
    cout << "Thread " << i << " start" << endl;

    int foo = global_int;

    cout << "Thread " << i << " end" << endl;

    return NULL;
}

int main() {
    for (uint64_t i = 0; i < 10; i++) {
        pthread_create(threads + i, NULL, func, (void*)i);
    }
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}