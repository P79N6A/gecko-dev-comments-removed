
















#ifndef insanity_pkix__ScopedPtr_h
#define insanity_pkix__ScopedPtr_h


#ifdef __GNUC__
#if __GNUC__ * 100 + __GNUC_MINOR__ < 406
  #define nullptr __null
#endif
#endif

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

template <typename T, void(*Destroyer)(T*)>
inline bool
operator==(T* a, const ScopedPtr<T, Destroyer>& b)
{
  return a == b.get();
}

template <typename T, void(*Destroyer)(T*)>
inline bool
operator==(const ScopedPtr<T, Destroyer>& a, T* b)
{
  return a.get() == b;
}

template <typename T, void(*Destroyer)(T*)>
inline bool
operator!=(T* a, const ScopedPtr<T, Destroyer>& b)
{
  return a != b.get();
}

template <typename T, void(*Destroyer)(T*)>
inline bool
operator!=(const ScopedPtr<T, Destroyer>& a, T* b)
{
  return a.get() != b;
}

} } 

#endif 
