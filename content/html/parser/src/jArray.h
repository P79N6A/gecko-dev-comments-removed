




































#ifndef jArray_h__
#define jArray_h__

#define J_ARRAY_STATIC(T, L, arr) \
  jArray<T,L>( ((T*)arr), (sizeof(arr)/sizeof(arr[0])) ) 

template<class T, class L> 
class jArray {
  private:
    T* arr;
  public:
    L length;
    jArray(T* const a, L const len);
    jArray(L const len);
    jArray(const jArray<T,L>& other);
    jArray();
    operator T*() { return arr; }
    T& operator[] (L const index) { return arr[index]; }
    void release() { delete[] arr; arr = 0; length = 0; }
    L binarySearch(T const elem);
};

template<class T, class L>
jArray<T,L>::jArray(T* const a, L const len)
       : arr(a), length(len)
{
}

template<class T, class L>
jArray<T,L>::jArray(L const len)
       : arr(new T[len]), length(len)
{
}

template<class T, class L>
jArray<T,L>::jArray(const jArray<T,L>& other)
       : arr(other.arr), length(other.length)
{
}

template<class T, class L>
jArray<T,L>::jArray()
       : arr(0), length(0)
{
}

template<class T, class L>
L
jArray<T,L>::binarySearch(T const elem)
{
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

#endif 