




#ifndef mozilla_EventForwards_h__
#define mozilla_EventForwards_h__

#include <stdint.h>









enum nsEventStatus
{
  
  nsEventStatus_eIgnore,
  
  nsEventStatus_eConsumeNoDefault,
  
  nsEventStatus_eConsumeDoDefault
};

namespace mozilla {

typedef uint16_t Modifiers;

#define NS_DEFINE_KEYNAME(aCPPName, aDOMKeyName) \
  KEY_NAME_INDEX_##aCPPName,

enum KeyNameIndex
{
#include "nsDOMKeyNameList.h"
  
  
  NUMBER_OF_KEY_NAME_INDEX
};

#undef NS_DEFINE_KEYNAME

} 





namespace mozilla {

#define NS_EVENT_CLASS(aPrefix, aName) class aPrefix##aName;
#define NS_ROOT_EVENT_CLASS(aPrefix, aName) NS_EVENT_CLASS(aPrefix, aName)

#include "mozilla/EventClassList.h"

#undef NS_EVENT_CLASS
#undef NS_ROOT_EVENT_CLASS


struct EventFlags;


struct AlternativeCharCode;
struct TextRangeStyle;
struct TextRange;

typedef TextRange* TextRangeArray;

} 

#endif 
