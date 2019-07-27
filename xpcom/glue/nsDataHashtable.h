





#ifndef nsDataHashtable_h__
#define nsDataHashtable_h__

#include "nsHashKeys.h"
#include "nsBaseHashtable.h"









template<class KeyClass, class DataType>
class nsDataHashtable
  : public nsBaseHashtable<KeyClass, DataType, DataType>
{
public:
  nsDataHashtable() {}
  explicit nsDataHashtable(uint32_t aInitLength)
    : nsBaseHashtable<KeyClass, DataType, DataType>(aInitLength)
  {
  }
};

#endif 
