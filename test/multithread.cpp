#include <pthread.h>
#include <bits/stdc++.h>

using namespace std;

pthread_t threads[10];

int global_int;
pthread_mutex_t _lo;

void* func(void* arg) {
    uint64_t i = (uint64_t)arg;
    #ifdef MUL
    #ifdef FIX
    if (i > 0) {
        pthread_join(threads[i - 1], NULL);
    }
    #endif
    #endif
    pthread_mutex_lock(&_lo);
    cout << "Thread " << i << " start" << endl;

    global_int++;

    cout << "Thread " << i << " end" << endl;

    pthread_mutex_unlock(&_lo);
    return NULL;
}

int main() {
    pthread_mutex_init(&_lo, NULL);
    for (uint64_t i = 0; i < 10; i++) {
        #ifdef MUL
        pthread_create(threads + i, NULL, func, (void*)i);
        #endif
    }
    for (int i = 0; i < 10; i++) {
        #ifdef MUL
        #ifndef FIX
        pthread_join(threads[i], NULL);
        #else
        if (i == 9) {
            pthread_join(threads[i], NULL);
        }
        #endif
        #else
        func((void*)i);
        #endif
    }
    return 0;
}