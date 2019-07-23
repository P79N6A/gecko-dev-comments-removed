




























#ifndef BASE_SINGLETON_OBJC_H_
#define BASE_SINGLETON_OBJC_H_

#import <Foundation/Foundation.h>
#include "base/singleton.h"





template<typename Type>
struct DefaultSingletonObjCTraits : public DefaultSingletonTraits<Type> {
  static Type* New() {
    return [[Type alloc] init];
  }

  static void Delete(Type* object) {
    [object release];
  }
};




template<typename Type,
         typename Traits = DefaultSingletonObjCTraits<Type>,
         typename DifferentiatingType = Type>
class SingletonObjC : public Singleton<Type, Traits, DifferentiatingType> {
};

#endif  
