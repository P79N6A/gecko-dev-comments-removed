



#include "base/compiler_specific.h"

#ifndef BASE_MOVE_H_
#define BASE_MOVE_H_





































































































































































































#define MOVE_ONLY_TYPE_FOR_CPP_03(type, rvalue_type) \
 private: \
  struct rvalue_type { \
    explicit rvalue_type(type* object) : object(object) {} \
    type* object; \
  }; \
  type(type&); \
  void operator=(type&); \
 public: \
  operator rvalue_type() { return rvalue_type(this); } \
  type Pass() WARN_UNUSED_RESULT { return type(rvalue_type(this)); } \
  typedef void MoveOnlyTypeForCPP03; \
 private:

#define MOVE_ONLY_TYPE_WITH_MOVE_CONSTRUCTOR_FOR_CPP_03(type) \
 private: \
  type(type&); \
  void operator=(type&); \
 public: \
  type&& Pass() WARN_UNUSED_RESULT { return static_cast<type&&>(*this); } \
  typedef void MoveOnlyTypeForCPP03; \
 private:

#endif  
