









#ifndef mozilla_DebugOnly_h_
#define mozilla_DebugOnly_h_

namespace mozilla {















template<typename T>
class DebugOnly
{
  public:
#ifdef DEBUG
    T value;

    DebugOnly() { }
    DebugOnly(const T& other) : value(other) { }
    DebugOnly(const DebugOnly& other) : value(other.value) { }
    DebugOnly& operator=(const T& rhs) {
      value = rhs;
      return *this;
    }
    void operator++(int) {
      value++;
    }
    void operator--(int) {
      value--;
    }

    T* operator&() { return &value; }

    operator T&() { return value; }
    operator const T&() const { return value; }

    T& operator->() { return value; }

#else
    DebugOnly() { }
    DebugOnly(const T&) { }
    DebugOnly(const DebugOnly&) { }
    DebugOnly& operator=(const T&) { return *this; }
    void operator++(int) { }
    void operator--(int) { }
#endif






    ~DebugOnly() {}
};

}

#endif  
