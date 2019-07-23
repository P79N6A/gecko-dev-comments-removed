



#ifndef CHROME_COMMON_NAVIGATION_TYPES_H_
#define CHROME_COMMON_NAVIGATION_TYPES_H_

#include "base/basictypes.h"



class NavigationType {
 public:
  enum Type {
    
    NEW_PAGE,

    
    
    EXISTING_PAGE,

    
    
    
    
    SAME_PAGE,

    
    
    
    
    IN_PAGE,

    
    
    
    NEW_SUBFRAME,

    
    
    
    
    
    
    
    
    
    AUTO_SUBFRAME,

    
    
    
    NAV_IGNORE,
  };

 private:
  
  NavigationType() {}

  DISALLOW_COPY_AND_ASSIGN(NavigationType);
};

#endif
