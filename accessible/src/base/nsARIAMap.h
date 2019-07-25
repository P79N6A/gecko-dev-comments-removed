






































#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

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

enum eStateValueType
{
  kBoolType,
  kMixedType
};

enum EDefaultStateRule
{
  
  eUseFirstState
};




enum eStateMapEntryID
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

class nsStateMapEntry
{
public:
  


  nsStateMapEntry();

  



  nsStateMapEntry(PRUint64 aDefaultState, PRUint64 aExclusingState);

  


  nsStateMapEntry(nsIAtom** aAttrName, eStateValueType aType,
                  PRUint64 aPermanentState,
                  PRUint64 aTrueState,
                  PRUint64 aFalseState = 0,
                  bool aDefinedIfAbsent = false);

  


  nsStateMapEntry(nsIAtom** aAttrName,
                  const char* aValue1, PRUint64 aState1,
                  const char* aValue2, PRUint64 aState2,
                  const char* aValue3 = 0, PRUint64 aState3 = 0);

  



  nsStateMapEntry(nsIAtom** aAttrName, EDefaultStateRule aDefaultStateRule,
                  const char* aValue1, PRUint64 aState1,
                  const char* aValue2, PRUint64 aState2,
                  const char* aValue3 = 0, PRUint64 aState3 = 0);

  







  static bool MapToStates(nsIContent* aContent, PRUint64* aState,
                            eStateMapEntryID aStateMapEntryID);

private:
  
  nsIAtom** mAttributeName;

  
  bool mIsToken;

  
  PRUint64 mPermanentState;

  
  const char* mValue1;
  PRUint64 mState1;

  const char* mValue2;
  PRUint64 mState2;

  const char* mValue3;
  PRUint64 mState3;

  
  PRUint64 mDefaultState;

  
  bool mDefinedIfAbsent;

  
  PRUint64 mExcludingState;
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
  
  
  
  
  
  
  eStateMapEntryID attributeMap1;
  eStateMapEntryID attributeMap2;
  eStateMapEntryID attributeMap3;
};










struct nsARIAMap
{
  


  static nsRoleMapEntry gWAIRoleMap[];
  static PRUint32 gWAIRoleMapLength;

  



  static nsRoleMapEntry gLandmarkRoleMap;

  




  static nsRoleMapEntry gEmptyRoleMap;

  


  static nsStateMapEntry gWAIStateMap[];

  



  static eStateMapEntryID gWAIUnivStateMap[];
  
  


  static nsAttributeCharacteristics gWAIUnivAttrMap[];
  static PRUint32 gWAIUnivAttrMapLength;

  



  static PRUint64 UniversalStatesFor(nsIContent* aContent)
  {
    PRUint64 state = 0;
    PRUint32 index = 0;
    while (nsStateMapEntry::MapToStates(aContent, &state, gWAIUnivStateMap[index]))
      index++;

    return state;
  }
};

#endif
