




































 
#include "nsAccessibleWrap.h"

#import "mozAccessible.h"














struct AccessibleWrapper {
  mozAccessible *object;
  AccessibleWrapper (nsAccessibleWrap *parent, Class classType) {
    object = (mozAccessible*)[[classType alloc] initWithAccessible:parent];
  }

  ~AccessibleWrapper () {
    
    
    [object expire];
    
    [object release];
  }

  mozAccessible* getNativeObject () {
    return object;
  }
 
  PRBool isIgnored () {
    return (PRBool)[object accessibilityIsIgnored];
  }
};
