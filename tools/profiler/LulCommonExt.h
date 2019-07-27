


























































#ifndef LulCommonExt_h
#define LulCommonExt_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <string>
#include <map>
#include <vector>
#include <cstddef>            

#include "mozilla/Assertions.h"

namespace lul {

using std::string;
using std::map;







class UniqueString;


const char* const FromUniqueString(const UniqueString*);


bool IsEmptyUniqueString(const UniqueString*);







class UniqueStringUniverse {
public:
  UniqueStringUniverse() {}
  ~UniqueStringUniverse();
  
  const UniqueString* ToUniqueString(string str);
private:
  map<string, UniqueString*> map_;
};






typedef struct {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t  data4[8];
} MDGUID;  

typedef MDGUID GUID;











































template <typename T>
class scoped_ptr {
 private:

  T* ptr;

  scoped_ptr(scoped_ptr const &);
  scoped_ptr & operator=(scoped_ptr const &);

 public:

  typedef T element_type;

  explicit scoped_ptr(T* p = 0): ptr(p) {}

  ~scoped_ptr() {
    delete ptr;
  }

  void reset(T* p = 0) {
    if (ptr != p) {
      delete ptr;
      ptr = p;
    }
  }

  T& operator*() const {
    MOZ_ASSERT(ptr != 0);
    return *ptr;
  }

  T* operator->() const  {
    MOZ_ASSERT(ptr != 0);
    return ptr;
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
  }

  T* get() const  {
    return ptr;
  }

  void swap(scoped_ptr & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = 0;
    return tmp;
  }

 private:

  
  template <typename U> bool operator==(scoped_ptr<U> const& p) const;
  template <typename U> bool operator!=(scoped_ptr<U> const& p) const;
};

template<typename T> inline
void swap(scoped_ptr<T>& a, scoped_ptr<T>& b) {
  a.swap(b);
}

template<typename T> inline
bool operator==(T* p, const scoped_ptr<T>& b) {
  return p == b.get();
}

template<typename T> inline
bool operator!=(T* p, const scoped_ptr<T>& b) {
  return p != b.get();
}





template<typename T>
class scoped_array {
 private:

  T* ptr;

  scoped_array(scoped_array const &);
  scoped_array & operator=(scoped_array const &);

 public:

  typedef T element_type;

  explicit scoped_array(T* p = 0) : ptr(p) {}

  ~scoped_array() {
    delete[] ptr;
  }

  void reset(T* p = 0) {
    if (ptr != p) {
      delete [] ptr;
      ptr = p;
    }
  }

  T& operator[](std::ptrdiff_t i) const {
    MOZ_ASSERT(ptr != 0);
    MOZ_ASSERT(i >= 0);
    return ptr[i];
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
  }

  T* get() const {
    return ptr;
  }

  void swap(scoped_array & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = 0;
    return tmp;
  }

 private:

  
  template <typename U> bool operator==(scoped_array<U> const& p) const;
  template <typename U> bool operator!=(scoped_array<U> const& p) const;
};

template<class T> inline
void swap(scoped_array<T>& a, scoped_array<T>& b) {
  a.swap(b);
}

template<typename T> inline
bool operator==(T* p, const scoped_array<T>& b) {
  return p == b.get();
}

template<typename T> inline
bool operator!=(T* p, const scoped_array<T>& b) {
  return p != b.get();
}




class ScopedPtrMallocFree {
 public:
  inline void operator()(void* x) const {
    free(x);
  }
};




template<typename T, typename FreeProc = ScopedPtrMallocFree>
class scoped_ptr_malloc {
 private:

  T* ptr;

  scoped_ptr_malloc(scoped_ptr_malloc const &);
  scoped_ptr_malloc & operator=(scoped_ptr_malloc const &);

 public:

  typedef T element_type;

  explicit scoped_ptr_malloc(T* p = 0): ptr(p) {}

  ~scoped_ptr_malloc() {
    free_((void*) ptr);
  }

  void reset(T* p = 0) {
    if (ptr != p) {
      free_((void*) ptr);
      ptr = p;
    }
  }

  T& operator*() const {
    MOZ_ASSERT(ptr != 0);
    return *ptr;
  }

  T* operator->() const {
    MOZ_ASSERT(ptr != 0);
    return ptr;
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
  }

  T* get() const {
    return ptr;
  }

  void swap(scoped_ptr_malloc & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = 0;
    return tmp;
  }

 private:

  
  template <typename U, typename GP>
  bool operator==(scoped_ptr_malloc<U, GP> const& p) const;
  template <typename U, typename GP>
  bool operator!=(scoped_ptr_malloc<U, GP> const& p) const;

  static FreeProc const free_;
};

template<typename T, typename FP>
FP const scoped_ptr_malloc<T,FP>::free_ = FP();

template<typename T, typename FP> inline
void swap(scoped_ptr_malloc<T,FP>& a, scoped_ptr_malloc<T,FP>& b) {
  a.swap(b);
}

template<typename T, typename FP> inline
bool operator==(T* p, const scoped_ptr_malloc<T,FP>& b) {
  return p == b.get();
}

template<typename T, typename FP> inline
bool operator!=(T* p, const scoped_ptr_malloc<T,FP>& b) {
  return p != b.get();
}










class Module {
public:
  
  typedef uint64_t Address;

  
  
  
  
  enum ExprHow {
    kExprInvalid = 1,
    kExprPostfix,
    kExprSimple,
    kExprSimpleMem
  };

  struct Expr {
    
    Expr(const UniqueString* ident, long offset, bool deref) {
      if (IsEmptyUniqueString(ident)) {
        Expr();
      } else {
        postfix_ = "";
        ident_ = ident;
        offset_ = offset;
        how_ = deref ? kExprSimpleMem : kExprSimple;
      }
    }

    
    Expr() {
      postfix_ = "";
      ident_ = nullptr;
      offset_ = 0;
      how_ = kExprInvalid;
    }

    
    
    
    std::string getExprPostfix() const {
      switch (how_) {
        case kExprPostfix:
          return postfix_;
        case kExprSimple:
        case kExprSimpleMem: {
          char buf[40];
          sprintf(buf, " %ld %c%s", labs(offset_), offset_ < 0 ? '-' : '+',
                                    how_ == kExprSimple ? "" : " ^");
          return std::string(FromUniqueString(ident_)) + std::string(buf);
        }
        case kExprInvalid:
        default:
          MOZ_ASSERT(0 && "getExprPostfix: invalid Module::Expr type");
          return "Expr::genExprPostfix: kExprInvalid";
      }
    }

    
    const UniqueString* ident_;
    
    long        offset_;
    
    std::string postfix_;
    
    ExprHow     how_;
  };

  
  
  
  
  
  
  typedef std::map<const UniqueString*, Expr> RuleMap;

  
  
  typedef std::map<Address, RuleMap> RuleChangeMap;

  
  
  
  struct StackFrameEntry {
    
    
    Address address, size;

    
    
    RuleMap initial_rules;

    
    
    
    
    RuleChangeMap rule_changes;
  };

  
  
  Module(const std::string &name, const std::string &os,
         const std::string &architecture, const std::string &id);
  ~Module();

private:

  
  std::string name_, os_, architecture_, id_;
};


}  

#endif 
