





#ifndef ds_PriorityQueue_h
#define ds_PriorityQueue_h

#include "js/Vector.h"

namespace js {









template <class T, class P,
          size_t MinInlineCapacity = 0,
          class AllocPolicy = TempAllocPolicy>
class PriorityQueue
{
    Vector<T, MinInlineCapacity, AllocPolicy> heap;

    PriorityQueue(const PriorityQueue &) MOZ_DELETE;
    PriorityQueue &operator=(const PriorityQueue &) MOZ_DELETE;

  public:

    explicit PriorityQueue(AllocPolicy ap = AllocPolicy())
      : heap(ap)
    {}

    bool reserve(size_t capacity) {
        return heap.reserve(capacity);
    }

    size_t length() const {
        return heap.length();
    }

    bool empty() const {
        return heap.empty();
    }

    T removeHighest() {
        T highest = heap[0];
        T last = heap.popCopy();
        if (!heap.empty()) {
            heap[0] = last;
            siftDown(0);
        }
        return highest;
    }

    bool insert(const T &v) {
        if (!heap.append(v))
            return false;
        siftUp(heap.length() - 1);
        return true;
    }

    void infallibleInsert(const T &v) {
        heap.infallibleAppend(v);
        siftUp(heap.length() - 1);
    }

  private:

    












    void siftDown(size_t n) {
        while (true) {
            size_t left = n * 2 + 1;
            size_t right = n * 2 + 2;

            if (left < heap.length()) {
                if (right < heap.length()) {
                    if (P::priority(heap[n]) < P::priority(heap[right]) &&
                        P::priority(heap[left]) < P::priority(heap[right]))
                    {
                        swap(n, right);
                        n = right;
                        continue;
                    }
                }

                if (P::priority(heap[n]) < P::priority(heap[left])) {
                    swap(n, left);
                    n = left;
                    continue;
                }
            }

            break;
        }
    }

    void siftUp(size_t n) {
        while (n > 0) {
            size_t parent = (n - 1) / 2;

            if (P::priority(heap[parent]) > P::priority(heap[n]))
                break;

            swap(n, parent);
            n = parent;
        }
    }

    void swap(size_t a, size_t b) {
        T tmp = heap[a];
        heap[a] = heap[b];
        heap[b] = tmp;
    }
};

}  

#endif 
