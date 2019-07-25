






































#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

#include "mozilla/a11y/ARIAStateMap.h"
#include "mozilla/a11y/Role.h"
#include "prtypes.h"

class nsIAtom;
class nsIContent;







enum EValueRule
{
  


  eNoValue,

  



  eHasValueMinMax
};








enum EActionRule
{
  eNoAction,
  eActivateAction,
  eClickAction,
  ePressAction,
  eCheckUncheckAction,
  eExpandAction,
  eJumpAction,
  eOpenCloseAction,
  eSelectAction,
  eSortAction,
  eSwitchAction
};








enum ELiveAttrRule
{
  eNoLiveAttr,
  eOffLiveAttr,
  ePoliteLiveAttr
};








const bool kUseMapRole = true;




const bool kUseNativeRole = false;











const PRUint8 ATTR_BYPASSOBJ  = 0x0001;





const PRUint8 ATTR_VALTOKEN   = 0x0010;




struct nsAttributeCharacteristics
{
  nsIAtom** attributeName;
  const PRUint8 characteristics;
};









#define kNoReqStates 0

enum EDefaultStateRule
{
  
  eUseFirstState
};







struct nsRoleMapEntry
{
  
  const char *roleString;
  
  
  mozilla::a11y::role role;
  
  
  bool roleRule;
  
  
  EValueRule valueRule;

  
  EActionRule actionRule;

  
  
  ELiveAttrRule liveAttRule;

  
  PRUint64 state;   

  
  
  
  
  
  mozilla::a11y::aria::EStateRule attributeMap1;
  mozilla::a11y::aria::EStateRule attributeMap2;
  mozilla::a11y::aria::EStateRule attributeMap3;
};










struct nsARIAMap
{
  


  static nsRoleMapEntry gWAIRoleMap[];
  static PRUint32 gWAIRoleMapLength;

  



  static nsRoleMapEntry gLandmarkRoleMap;

  




  static nsRoleMapEntry gEmptyRoleMap;

  



  static mozilla::a11y::aria::EStateRule gWAIUnivStateMap[];

  


  static nsAttributeCharacteristics gWAIUnivAttrMap[];
  static PRUint32 gWAIUnivAttrMapLength;

  



  static PRUint64 UniversalStatesFor(mozilla::dom::Element* aElement)
  {
    PRUint64 state = 0;
    PRUint32 index = 0;
    while (mozilla::a11y::aria::MapToState(gWAIUnivStateMap[index],
                                           aElement, &state))
      index++;

    return state;
  }
};

#endif
