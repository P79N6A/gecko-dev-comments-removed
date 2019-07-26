








#ifndef SkTSort_DEFINED
#define SkTSort_DEFINED

#include "SkTypes.h"
#include "SkMath.h"


template <typename T> struct SkTCompareLT {
    bool operator()(const T a, const T b) const { return a < b; }
};


template <typename T> struct SkTPointerCompareLT {
    bool operator()(const T* a, const T* b) const { return *a < *b; }
};

















template <typename T, typename C>
void SkTHeapSort_SiftUp(T array[], size_t root, size_t bottom, C lessThan) {
    T x = array[root-1];
    size_t start = root;
    size_t j = root << 1;
    while (j <= bottom) {
        if (j < bottom && lessThan(array[j-1], array[j])) {
            ++j;
        }
        array[root-1] = array[j-1];
        root = j;
        j = root << 1;
    }
    j = root >> 1;
    while (j >= start) {
        if (lessThan(array[j-1], x)) {
            array[root-1] = array[j-1];
            root = j;
            j = root >> 1;
        } else {
            break;
        }
    }
    array[root-1] = x;
}









template <typename T, typename C>
void SkTHeapSort_SiftDown(T array[], size_t root, size_t bottom, C lessThan) {
    T x = array[root-1];
    size_t child = root << 1;
    while (child <= bottom) {
        if (child < bottom && lessThan(array[child-1], array[child])) {
            ++child;
        }
        if (lessThan(x, array[child-1])) {
            array[root-1] = array[child-1];
            root = child;
            child = root << 1;
        } else {
            break;
        }
    }
    array[root-1] = x;
}








template <typename T, typename C> void SkTHeapSort(T array[], size_t count, C lessThan) {
    for (size_t i = count >> 1; i > 0; --i) {
        SkTHeapSort_SiftDown(array, i, count, lessThan);
    }

    for (size_t i = count - 1; i > 0; --i) {
        SkTSwap<T>(array[0], array[i]);
        SkTHeapSort_SiftUp(array, 1, i, lessThan);
    }
}


template <typename T> void SkTHeapSort(T array[], size_t count) {
    SkTHeapSort(array, count, SkTCompareLT<T>());
}




template <typename T, typename C> static void SkTInsertionSort(T* left, T* right, C lessThan) {
    for (T* next = left + 1; next <= right; ++next) {
        T insert = *next;
        T* hole = next;
        while (left < hole && lessThan(insert, *(hole - 1))) {
            *hole = *(hole - 1);
            --hole;
        }
        *hole = insert;
    }
}



template <typename T, typename C>
static T* SkTQSort_Partition(T* left, T* right, T* pivot, C lessThan) {
    T pivotValue = *pivot;
    SkTSwap(*pivot, *right);
    T* newPivot = left;
    while (left < right) {
        if (lessThan(*left, pivotValue)) {
            SkTSwap(*left, *newPivot);
            newPivot += 1;
        }
        left += 1;
    }
    SkTSwap(*newPivot, *right);
    return newPivot;
}













template <typename T, typename C> void SkTIntroSort(int depth, T* left, T* right, C lessThan) {
    while (true) {
        if (right - left < 32) {
            SkTInsertionSort(left, right, lessThan);
            return;
        }

        if (depth == 0) {
            SkTHeapSort<T>(left, right - left + 1, lessThan);
            return;
        }
        --depth;

        T* pivot = left + ((right - left) >> 1);
        pivot = SkTQSort_Partition(left, right, pivot, lessThan);

        SkTIntroSort(depth, left, pivot - 1, lessThan);
        left = pivot + 1;
    }
}








template <typename T, typename C> void SkTQSort(T* left, T* right, C lessThan) {
    if (left >= right) {
        return;
    }
    
    int depth = 2 * SkNextLog2(SkToU32(right - left));
    SkTIntroSort(depth, left, right, lessThan);
}


template <typename T> void SkTQSort(T* left, T* right) {
    SkTQSort(left, right, SkTCompareLT<T>());
}


template <typename T> void SkTQSort(T** left, T** right) {
    SkTQSort(left, right, SkTPointerCompareLT<T>());
}


template <typename T, int (COMPARE)(const T*, const T*)>
class SkTSearchCompareLTFunctor {
public:
    bool operator()(const T& a, const T& b) {
        return COMPARE(&a, &b) < 0;
    }
};


#endif
