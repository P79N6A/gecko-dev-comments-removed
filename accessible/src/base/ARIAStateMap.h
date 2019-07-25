





#ifndef _mozilla_a11y_aria_ARIAStateMap_h_
#define _mozilla_a11y_aria_ARIAStateMap_h_

#include "prtypes.h"

namespace mozilla {

namespace dom {
class Element;
}

namespace a11y {
namespace aria {




enum EStateRule
{
  eARIANone,
  eARIAAutoComplete,
  eARIABusy,
  eARIACheckableBool,
  eARIACheckableMixed,
  eARIACheckedMixed,
  eARIADisabled,
  eARIAExpanded,
  eARIAHasPopup,
  eARIAInvalid,
  eARIAMultiline,
  eARIAMultiSelectable,
  eARIAOrientation,
  eARIAPressed,
  eARIAReadonly,
  eARIAReadonlyOrEditable,
  eARIARequired,
  eARIASelectable,
  eReadonlyUntilEditable
};










bool MapToState(EStateRule aRule, dom::Element* aElement, PRUint64* aState);

} 
} 
} 

#endif
