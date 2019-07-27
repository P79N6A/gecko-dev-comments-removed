





#ifndef nsJSThingHashtable_h__
#define nsJSThingHashtable_h__

#include "nsHashKeys.h"
#include "nsBaseHashtable.h"

namespace JS {
template<class T>
class Heap;
} 










template<class T>
class nsHashKeyDisallowMemmove : public T
{
public:
  explicit nsHashKeyDisallowMemmove(const typename T::KeyTypePointer aKey) : T(aKey) {}
  enum { ALLOW_MEMMOVE = false };
};




















template<class KeyClass, class DataType>
class nsJSThingHashtable
  : public nsBaseHashtable<nsHashKeyDisallowMemmove<KeyClass>,
                           JS::Heap<DataType>, DataType>
{
};

#endif 
