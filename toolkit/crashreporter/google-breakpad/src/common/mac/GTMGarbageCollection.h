

















#import <Foundation/Foundation.h>

#import "GTMDefines.h"









#if (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5) && !GTM_IPHONE_SDK





#define GTMNSMakeCollectable(cf) ((id)NSMakeCollectable(cf))



GTM_INLINE void GTMNSMakeUncollectable(id object) {
  [[NSGarbageCollector defaultCollector] disableCollectorForPointer:object];
}






GTM_INLINE BOOL GTMIsGarbageCollectionEnabled(void) {
  return ([NSGarbageCollector defaultCollector] != nil);
}

#else

#define GTMNSMakeCollectable(cf) ((id)(cf))

GTM_INLINE void GTMNSMakeUncollectable(id object) {
}

GTM_INLINE BOOL GTMIsGarbageCollectionEnabled(void) {
  return NO;
}

#endif





#define GTMCFAutorelease(cf) ([GTMNSMakeCollectable(cf) autorelease])

