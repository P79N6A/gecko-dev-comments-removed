



#ifndef SANDBOX_SRC_POLICY_ENGINE_PARAMS_H__
#define SANDBOX_SRC_POLICY_ENGINE_PARAMS_H__

#include "base/basictypes.h"
#include "sandbox/win/src/internal_types.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_nt_util.h"





namespace sandbox {









































class ParameterSet {
 public:
  ParameterSet() : real_type_(INVALID_TYPE), address_(NULL) {}

  
  bool Get(unsigned long* destination) const {
    if (ULONG_TYPE != real_type_) {
      return false;
    }
    *destination = Void2TypePointerCopy<unsigned long>();
    return true;
  }

  
  bool Get(const void** destination) const {
    if (VOIDPTR_TYPE != real_type_) {
      return false;
    }
    *destination = Void2TypePointerCopy<void*>();
    return true;
  }

  
  bool Get(const wchar_t** destination) const {
    if (WCHAR_TYPE != real_type_) {
      return false;
    }
    *destination = Void2TypePointerCopy<const wchar_t*>();
    return true;
  }

  
  bool IsValid() const {
    return INVALID_TYPE != real_type_;
  }

 protected:
  
  
  ParameterSet(ArgType real_type, const void* address)
      : real_type_(real_type), address_(address) {
  }

 private:
  
  
  template <typename T>
  T Void2TypePointerCopy() const {
    return *(reinterpret_cast<const T*>(address_));
  }

  ArgType real_type_;
  const void* address_;
};







template <typename T>
class ParameterSetEx : public ParameterSet {
 public:
  ParameterSetEx(const void* address);
};

template<>
class ParameterSetEx<void const*> : public ParameterSet {
 public:
  ParameterSetEx(const void* address)
      : ParameterSet(VOIDPTR_TYPE, address) {}
};

template<>
class ParameterSetEx<void*> : public ParameterSet {
 public:
  ParameterSetEx(const void* address)
      : ParameterSet(VOIDPTR_TYPE, address) {}
};


template<>
class ParameterSetEx<wchar_t*> : public ParameterSet {
 public:
  ParameterSetEx(const void* address)
      : ParameterSet(WCHAR_TYPE, address) {}
};

template<>
class ParameterSetEx<wchar_t const*> : public ParameterSet {
 public:
  ParameterSetEx(const void* address)
      : ParameterSet(WCHAR_TYPE, address) {}
};


template<>
class ParameterSetEx<unsigned long> : public ParameterSet {
 public:
  ParameterSetEx(const void* address)
      : ParameterSet(ULONG_TYPE, address) {}
};

template<>
class ParameterSetEx<UNICODE_STRING> : public ParameterSet {
 public:
  ParameterSetEx(const void* address)
      : ParameterSet(UNISTR_TYPE, address) {}
};

template <typename T>
ParameterSet ParamPickerMake(T& parameter) {
  return ParameterSetEx<T>(&parameter);
};

struct CountedParameterSetBase {
  int count;
  ParameterSet parameters[1];
};





template <typename T>
struct CountedParameterSet {
  CountedParameterSet() : count(T::PolParamLast) {}

  ParameterSet& operator[](typename T::Args n) {
    return parameters[n];
  }

  CountedParameterSetBase* GetBase() {
    return reinterpret_cast<CountedParameterSetBase*>(this);
  }

  int count;
  ParameterSet parameters[T::PolParamLast];
};

}  

#endif  
