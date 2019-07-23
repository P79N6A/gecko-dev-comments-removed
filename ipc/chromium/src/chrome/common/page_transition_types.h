



#ifndef CHROME_COMMON_PAGE_TRANSITION_TYPES_H__
#define CHROME_COMMON_PAGE_TRANSITION_TYPES_H__

#include "base/basictypes.h"
#include "base/logging.h"


class PageTransition {
 public:
  
  
  
  
  
  
  
  
  
  
  enum {
    
    LINK = 0,

    
    
    
    
    
    TYPED = 1,

    
    
    AUTO_BOOKMARK = 2,

    
    
    
    
    
    AUTO_SUBFRAME = 3,

    
    
    
    
    
    MANUAL_SUBFRAME = 4,

    
    
    
    
    
    
    GENERATED = 5,

    
    START_PAGE = 6,

    
    
    
    FORM_SUBMIT = 7,

    
    
    
    
    
    
    
    
    
    
    RELOAD = 8,

    
    
    
    
    
    
    
    
    
    KEYWORD = 9,

    
    
    KEYWORD_GENERATED = 10,

    
    
    LAST_CORE = KEYWORD_GENERATED,
    CORE_MASK = 0xFF,

    
    
    

    
    CHAIN_START = 0x10000000,

    
    CHAIN_END = 0x20000000,

    
    CLIENT_REDIRECT = 0x40000000,

    
    
    
    SERVER_REDIRECT = 0x80000000,

    
    IS_REDIRECT_MASK = 0xC0000000,

    
    QUALIFIER_MASK = 0xFFFFFF00
  };

  
  typedef unsigned int Type;

  static bool ValidType(int32 type) {
    Type t = StripQualifier(static_cast<Type>(type));
    return (t >= 0 && t <= LAST_CORE);
  }

  static Type FromInt(int32 type) {
    if (!ValidType(type)) {
      NOTREACHED() << "Invalid transition type " << type;

      
      return LINK;
    }
    return static_cast<Type>(type);
  }

  
  
  static bool IsMainFrame(Type type) {
    int32 t = StripQualifier(type);
    return (t != AUTO_SUBFRAME && t != MANUAL_SUBFRAME);
  }

  
  static bool IsRedirect(Type type) {
    return (type & IS_REDIRECT_MASK) != 0;
  }

  
  static Type StripQualifier(Type type) {
    return static_cast<Type>(type & ~QUALIFIER_MASK);
  }

  
  static int32 GetQualifier(Type type) {
    return type & QUALIFIER_MASK;
  }
};

#endif
