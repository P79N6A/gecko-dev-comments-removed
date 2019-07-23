



#ifndef BASE_SCOPED_NSOBJECT_H_
#define BASE_SCOPED_NSOBJECT_H_

#import <Foundation/Foundation.h>
#include "base/basictypes.h"











template<typename NST>
class scoped_nsobject {
 public:
  typedef NST* element_type;

  explicit scoped_nsobject(NST* object = nil)
      : object_(object) {
  }

  ~scoped_nsobject() {
    [object_ release];
  }

  void reset(NST* object = nil) {
    [object_ release];
    object_ = object;
  }

  bool operator==(NST* that) const {
    return object_ == that;
  }

  bool operator!=(NST* that) const {
    return object_ != that;
  }

  operator NST*() const {
    return object_;
  }

  NST* get() const {
    return object_;
  }

  void swap(scoped_nsobject& that) {
    NST* temp = that.object_;
    that.object_ = object_;
    object_ = temp;
  }

  
  
  
  NST* release() {
    NST* temp = object_;
    object_ = nil;
    return temp;
  }

 private:
  NST* object_;

  DISALLOW_COPY_AND_ASSIGN(scoped_nsobject);
};

#endif  
