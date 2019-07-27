





















#ifndef jArray_h
#define jArray_h

#include "mozilla/Attributes.h"
#include "mozilla/BinarySearch.h"
#include "nsDebug.h"

template<class T, class L>
struct staticJArray {
  const T* arr;
  const L length;
  operator T*() { return arr; }
  T& operator[] (L const index) { return ((T*)arr)[index]; }
  L binarySearch(T const elem) {
    size_t idx;
    bool found = mozilla::BinarySearch(arr, 0, length, elem, &idx);
    return found ? idx : -1;
  }
};

template<class T, class L>
struct jArray {
  T* arr;
  L length;
  static jArray<T,L> newJArray(L const len) {
    NS_ASSERTION(len >= 0, "Bad length.");
    jArray<T,L> newArray = { new T[len], len };
    return newArray;
  }
  operator T*() { return arr; }
  T& operator[] (L const index) { return arr[index]; }
  void operator=(staticJArray<T,L>& other) {
    arr = (T*)other.arr;
    length = other.length;
  }
};

template<class T, class L>
class autoJArray {
  private:
    T* arr;
  public:
    L length;
    autoJArray()
     : arr(0)
     , length(0)
    {
    }
    MOZ_IMPLICIT autoJArray(const jArray<T,L>& other)
     : arr(other.arr)
     , length(other.length)
    {
    }
    ~autoJArray()
    {
      delete[] arr;
    }
    operator T*() { return arr; }
    T& operator[] (L const index) { return arr[index]; }
    operator jArray<T,L>() {
      
      
      
      jArray<T,L> newArray = { arr, length };
      return newArray;
    }
    void operator=(const jArray<T,L>& other) {
      delete[] arr;
      arr = other.arr;
      length = other.length;
    }
    void operator=(decltype(nullptr)) {
      
      delete[] arr;
      arr = nullptr;
      length = 0;
    }
};

#endif 
