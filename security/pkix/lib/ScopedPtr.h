























#ifndef mozilla_pkix_ScopedPtr_h
#define mozilla_pkix_ScopedPtr_h

namespace mozilla { namespace pkix {






template <typename T, void (&Destroyer)(T*)>
class ScopedPtr final
{
public:
  explicit ScopedPtr(T* value = nullptr) : mValue(value) { }

  ScopedPtr(const ScopedPtr&) = delete;

  ~ScopedPtr()
  {
    if (mValue) {
      Destroyer(mValue);
    }
  }

  void operator=(const ScopedPtr&) = delete;

  T& operator*() const { return *mValue; }
  T* operator->() const { return mValue; }

  explicit operator bool() const { return mValue; }

  T* get() const { return mValue; }

  T* release()
  {
    T* result = mValue;
    mValue = nullptr;
    return result;
  }

  void reset(T* newValue = nullptr)
  {
    
    
    T* oldValue = mValue;
    mValue = newValue;
    if (oldValue) {
      Destroyer(oldValue);
    }
  }

private:
  T* mValue;
};

} } 

#endif 
