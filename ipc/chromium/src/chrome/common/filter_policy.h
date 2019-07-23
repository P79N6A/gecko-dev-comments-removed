



#ifndef CHROME_COMMON_FILTER_POLICY_H__
#define CHROME_COMMON_FILTER_POLICY_H__

#include "base/basictypes.h"





class FilterPolicy {
 public:
  enum Type {
    
    DONT_FILTER = 0,

    
    
    
    
    
    FILTER_ALL_EXCEPT_IMAGES,

    
    FILTER_ALL
  };

  static bool ValidType(int32 type) {
    return type >= DONT_FILTER && type <= FILTER_ALL;
  }

  static Type FromInt(int32 type) {
    return static_cast<Type>(type);
  }

 private:
  
  FilterPolicy();
  ~FilterPolicy();

};

#endif
