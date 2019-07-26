







#ifndef mozilla_Char16_h
#define mozilla_Char16_h

#ifdef __cplusplus







#ifdef _MSC_VER
   











#  define MOZ_UTF16_HELPER(s) L##s
#  define _CHAR16T
typedef wchar_t char16_t;
typedef unsigned int char32_t;
#else
   
#  define MOZ_UTF16_HELPER(s) u##s
   



#  define MOZ_CHAR16_IS_NOT_WCHAR
#  ifdef WIN32
#    define MOZ_USE_CHAR16_WRAPPER
#  endif
#endif

#ifdef MOZ_USE_CHAR16_WRAPPER
# include <string>
  









class char16ptr_t
{
private:
  const char16_t* mPtr;
  static_assert(sizeof(char16_t) == sizeof(wchar_t),
                "char16_t and wchar_t sizes differ");

public:
  char16ptr_t(const char16_t* aPtr) : mPtr(aPtr) {}
  char16ptr_t(const wchar_t* aPtr) :
    mPtr(reinterpret_cast<const char16_t*>(aPtr))
  {}

  
  constexpr char16ptr_t(decltype(nullptr)) : mPtr(nullptr) {}

  operator const char16_t*() const
  {
    return mPtr;
  }
  operator const wchar_t*() const
  {
    return reinterpret_cast<const wchar_t*>(mPtr);
  }
  operator const void*() const
  {
    return mPtr;
  }
  operator bool() const
  {
    return mPtr != nullptr;
  }
  operator std::wstring() const
  {
    return std::wstring(static_cast<const wchar_t*>(*this));
  }

  
  explicit operator char16_t*() const
  {
    return const_cast<char16_t*>(mPtr);
  }
  explicit operator wchar_t*() const
  {
    return const_cast<wchar_t*>(static_cast<const wchar_t*>(*this));
  }

  




  explicit operator const char*() const
  {
    return reinterpret_cast<const char*>(mPtr);
  }
  explicit operator const unsigned char*() const
  {
    return reinterpret_cast<const unsigned char*>(mPtr);
  }
  explicit operator unsigned char*() const
  {
    return const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(mPtr));
  }
  explicit operator void*() const
  {
    return const_cast<char16_t*>(mPtr);
  }

  
  char16_t operator[](size_t aIndex) const
  {
    return mPtr[aIndex];
  }
  bool operator==(const char16ptr_t &aOther) const
  {
    return mPtr == aOther.mPtr;
  }
  bool operator==(decltype(nullptr)) const
  {
    return mPtr == nullptr;
  }
  bool operator!=(const char16ptr_t &aOther) const
  {
    return mPtr != aOther.mPtr;
  }
  bool operator!=(decltype(nullptr)) const
  {
    return mPtr != nullptr;
  }
  char16ptr_t operator+(size_t aValue) const
  {
    return char16ptr_t(mPtr + aValue);
  }
  ptrdiff_t operator-(const char16ptr_t &aOther) const
  {
    return mPtr - aOther.mPtr;
  }
};

inline decltype((char*)0-(char*)0)
operator-(const char16_t* aX, const char16ptr_t aY)
{
  return aX - static_cast<const char16_t*>(aY);
}

#else

typedef const char16_t* char16ptr_t;

#endif









#define MOZ_UTF16(s) MOZ_UTF16_HELPER(s)

static_assert(sizeof(char16_t) == 2, "Is char16_t type 16 bits?");
static_assert(char16_t(-1) > char16_t(0), "Is char16_t type unsigned?");
static_assert(sizeof(MOZ_UTF16('A')) == 2, "Is char literal 16 bits?");
static_assert(sizeof(MOZ_UTF16("")[0]) == 2, "Is string char 16 bits?");

#endif

#endif 
