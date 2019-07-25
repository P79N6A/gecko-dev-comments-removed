




































 
#include "nsAccessibleWrap.h"

#include "nsObjCExceptions.h"

#import "mozAccessible.h"














struct AccessibleWrapper {
  mozAccessible *object;
  AccessibleWrapper (nsAccessibleWrap *parent, Class classType) {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

    object = (mozAccessible*)[[classType alloc] initWithAccessible:parent];

    NS_OBJC_END_TRY_ABORT_BLOCK;
  }

  ~AccessibleWrapper () {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK;

    
    
    [object expire];
    
    [object release];

    NS_OBJC_END_TRY_ABORT_BLOCK;
  }

  mozAccessible* getNativeObject () {
    return object;
  }
 
  bool isIgnored () {
    NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN;

    return (bool)[object accessibilityIsIgnored];

    NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(PR_FALSE);
  }
};
