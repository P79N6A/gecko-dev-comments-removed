



#ifndef BASE_FOUNDATION_UTILS_MAC_H_
#define BASE_FOUNDATION_UTILS_MAC_H_

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
















static inline id CFTypeRefToNSObjectAutorelease(CFTypeRef cf_object) {
  
  
  
  
  
  
  
  return [NSMakeCollectable(cf_object) autorelease];
}

#endif  
