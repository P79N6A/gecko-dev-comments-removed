





















#ifndef jArray_h_
#define jArray_h_

#include "mozilla/NullPtr.h"
#include "nsDebug.h"

template<class T, class L>
struct staticJArray {
  const T* arr;
  const L length;
  operator T*() { return arr; }
  T& operator[] (L const index) { return ((T*)arr)[index]; }
  L binarySearch(T const elem) {
    L lo = 0;
    L hi = length - 1;
    while (lo <= hi) {
      L mid = (lo + hi) / 2;
      if (arr[mid] > elem) {
        hi = mid - 1;
      } else if (arr[mid] < elem) {
        lo = mid + 1;
      } else {
        return mid;
      }
    }
    return -1;
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
    autoJArray(const jArray<T,L>& other)
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
#if defined(MOZ_HAVE_CXX11_NULLPTR)
#  if defined(__clang__)
    
    typedef decltype(nullptr) jArray_nullptr_t;
#  else
    
    typedef std::nullptr_t jArray_nullptr_t;
#  endif
#elif defined(__GNUC__)
    typedef void* jArray_nullptr_t;
#elif defined(_WIN64)
    typedef uint64_t jArray_nullptr_t;
#else
    typedef uint32_t jArray_nullptr_t;
#endif
    void operator=(jArray_nullptr_t zero) {
      
      
      delete[] arr;
      arr = nullptr;
      length = 0;
    }
};

#endif 
