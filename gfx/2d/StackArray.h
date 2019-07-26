






template <class T, size_t size>
class StackArray
{
public:
  StackArray(size_t count) {
    if (count > size) {
      mData = new T[count];
    } else {
      mData = mStackData;
    }
  }
  ~StackArray() {
    if (mData != mStackData) {
      delete[] mData;
    }
  }
  T& operator[](size_t n) { return mData[n]; }
  const T& operator[](size_t n) const { return mData[n]; }
  T* data() { return mData; };
private:
  T mStackData[size];
  T* mData;
};
