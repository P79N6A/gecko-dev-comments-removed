




































#ifndef nsDataHashtable_h__
#define nsDataHashtable_h__

#include "nsHashKeys.h"
#include "nsBaseHashtable.h"









template<class KeyClass,class DataType>
class nsDataHashtable :
  public nsBaseHashtable<KeyClass,DataType,DataType>
{ };

template<class KeyClass,class DataType>
class nsDataHashtableMT :
  public nsBaseHashtableMT<KeyClass,DataType,DataType>
{ };

#endif 
