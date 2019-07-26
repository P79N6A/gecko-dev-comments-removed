



#ifndef BASE_SCOPED_BSTR_WIN_H_
#define BASE_SCOPED_BSTR_WIN_H_

#include "base/basictypes.h"  

#include "base/logging.h"

#include <windows.h>
#include <oleauto.h>



class ScopedBstr {
 public:
  ScopedBstr() : bstr_(NULL) {
  }

  
  
  
  explicit ScopedBstr(const wchar_t* non_bstr);
  ~ScopedBstr();

  
  
  void Reset(BSTR bstr = NULL);

  
  BSTR Release();

  
  
  
  
  BSTR Allocate(const wchar_t* wide_str);

  
  
  BSTR AllocateBytes(int bytes);

  
  
  
  
  
  
  
  
  
  
  void SetByteLen(uint32_t bytes);

  
  void Swap(ScopedBstr& bstr2);

  
  
  
  
  BSTR* Receive();

  
  uint32_t Length() const;

  
  uint32_t ByteLength() const;

  operator BSTR() const {
    return bstr_;
  }

 protected:
  BSTR bstr_;

 private:
  
  
  bool operator==(const ScopedBstr& bstr2) const;
  bool operator!=(const ScopedBstr& bstr2) const;
  DISALLOW_COPY_AND_ASSIGN(ScopedBstr);
};




template <uint32_t string_bytes>
class StackBstrT {
 public:
  
  
  
  
  explicit StackBstrT(const wchar_t* const str) {
    
    
    COMPILE_ASSERT(sizeof(uint32_t) == sizeof(UINT), UintToUint32);
    COMPILE_ASSERT(sizeof(wchar_t) == sizeof(OLECHAR), WcharToOlechar);

    
    
    
    DCHECK(lstrlenW(str) == (string_bytes / sizeof(bstr_.str_[0])) - 1) <<
        "not expecting a string pointer";
    memcpy(bstr_.str_, str, string_bytes);
    bstr_.len_ = string_bytes - sizeof(wchar_t);
  }

  operator BSTR() {
    return bstr_.str_;
  }

 protected:
  struct BstrInternal {
    uint32_t len_;
    wchar_t str_[string_bytes / sizeof(wchar_t)];
  } bstr_;
};








#define StackBstr(str) \
  static_cast<BSTR>(StackBstrT<sizeof(str)>(str))








#define StackBstrVar(str, var) \
  StackBstrT<sizeof(str)> var(str)

#endif  
