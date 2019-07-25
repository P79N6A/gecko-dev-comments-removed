



#ifndef mozilla_a11y_Filters_h__
#define mozilla_a11y_Filters_h__

#include "mozilla/StandardInteger.h"

class Accessible;




namespace mozilla {
namespace a11y {
namespace filters {

enum EResult {
  eSkip = 0,
  eMatch = 1,
  eSkipSubtree = 2
};




typedef uint32_t (*FilterFuncPtr) (Accessible*);




uint32_t GetSelected(Accessible* aAccessible);
uint32_t GetSelectable(Accessible* aAccessible);




uint32_t GetRow(Accessible* aAccessible);




uint32_t GetCell(Accessible* aAccessible);




uint32_t GetEmbeddedObject(Accessible* aAccessible);

} 
} 
} 

#endif
