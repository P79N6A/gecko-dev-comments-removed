










#ifndef mozilla_DebugOnly_h
#define mozilla_DebugOnly_h

namespace mozilla {


















template<typename T>
class DebugOnly
{
public:
#ifdef DEBUG
  T value;

  DebugOnly() { }
  DebugOnly(const T& aOther) : value(aOther) { }
  DebugOnly(const DebugOnly& aOther) : value(aOther.value) { }
  DebugOnly& operator=(const T& aRhs) {
    value = aRhs;
    return *this;
  }

  void operator++(int) { value++; }
  void operator--(int) { value--; }

  T* operator&() { return &value; }

  operator T&() { return value; }
  operator const T&() const { return value; }

  T& operator->() { return value; }
  const T& operator->() const { return value; }

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
