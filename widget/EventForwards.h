




#ifndef mozilla_EventForwards_h__
#define mozilla_EventForwards_h__

#include <stdint.h>

#include "mozilla/TypedEnum.h"









enum nsEventStatus
{
  
  nsEventStatus_eIgnore,
  
  nsEventStatus_eConsumeNoDefault,
  
  nsEventStatus_eConsumeDoDefault
};

namespace mozilla {

typedef uint8_t EventClassIDType;

enum EventClassID MOZ_ENUM_TYPE(EventClassIDType)
{
  
  
  
#define NS_ROOT_EVENT_CLASS(aPrefix, aName)   eBasic##aName##Class
#define NS_EVENT_CLASS(aPrefix, aName)      , e##aName##Class

#include "mozilla/EventClassList.h"

#undef NS_EVENT_CLASS
#undef NS_ROOT_EVENT_CLASS
};

typedef uint16_t Modifiers;

#define NS_DEFINE_KEYNAME(aCPPName, aDOMKeyName) \
  KEY_NAME_INDEX_##aCPPName,

enum KeyNameIndex
{
#include "mozilla/KeyNameList.h"
  
  
  KEY_NAME_INDEX_USE_STRING
};

#undef NS_DEFINE_KEYNAME

#define NS_DEFINE_PHYSICAL_KEY_CODE_NAME(aCPPName, aDOMCodeName) \
  CODE_NAME_INDEX_##aCPPName,

enum CodeNameIndex
{
#include "mozilla/PhysicalKeyCodeNameList.h"
  
  
  CODE_NAME_INDEX_USE_STRING
};

#undef NS_DEFINE_PHYSICAL_KEY_CODE_NAME

#define NS_DEFINE_COMMAND(aName, aCommandStr) , Command##aName

typedef int8_t CommandInt;
enum Command MOZ_ENUM_TYPE(CommandInt)
{
  CommandDoNothing

#include "mozilla/CommandList.h"
};
#undef NS_DEFINE_COMMAND

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

class TextRangeArray;

} 

#endif 
