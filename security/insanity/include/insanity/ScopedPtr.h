
















#ifndef insanity_pkix__ScopedPtr_h
#define insanity_pkix__ScopedPtr_h

namespace insanity { namespace pkix {



template <typename T, void (*Destroyer)(T*)>
class ScopedPtr
{
public:
  explicit ScopedPtr(T* value = nullptr) : mValue(value) { }
  ~ScopedPtr()
  {
    if (mValue) {
      Destroyer(mValue);
    }
  }

  void operator=(T* newValue)
  {
    if (mValue) {
      Destroyer(mValue);
    }
    mValue = newValue;
  }

  T& operator*() const { return *mValue; }
  T* operator->() const { return mValue; }
  operator bool() const { return mValue; }

  T* get() const { return mValue; }

  T* release()
  {
    T* result = mValue;
    mValue = nullptr;
    return result;
  }

  void reset() { *this = nullptr; }

protected:
  T* mValue;

  ScopedPtr(const ScopedPtr&) ;
  void operator=(const ScopedPtr&) ;
};

} } 

#endif 
