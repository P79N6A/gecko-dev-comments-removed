






































#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

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








const PRBool kUseMapRole = PR_TRUE;




const PRBool kUseNativeRole = PR_FALSE;











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
  eARIASelectable
};

class nsStateMapEntry
{
public:
  


  nsStateMapEntry();

  


  nsStateMapEntry(nsIAtom **aAttrName, eStateValueType aType,
                  PRUint32 aPermanentState,
                  PRUint32 aTrueState, PRUint32 aTrueExtraState,
                  PRUint32 aFalseState = 0, PRUint32 aFalseExtraState = 0,
                  PRBool aDefinedIfAbsent = PR_FALSE);

  


  nsStateMapEntry(nsIAtom **aAttrName,
                  const char *aValue1, PRUint32 aState1, PRUint32 aExtraState1,
                  const char *aValue2, PRUint32 aState2, PRUint32 aExtraState2,
                  const char *aValue3 = 0, PRUint32 aState3 = 0,
                  PRUint32 aExtraState3 = 0);

  



  nsStateMapEntry(nsIAtom **aAttrName,
                  EDefaultStateRule aDefaultStateRule,
                  const char *aValue1, PRUint32 aState1, PRUint32 aExtraState1,
                  const char *aValue2, PRUint32 aState2, PRUint32 aExtraState2,
                  const char *aValue3 = 0, PRUint32 aState3 = 0,
                  PRUint32 aExtraState3 = 0);

  








  static PRBool MapToStates(nsIContent *aContent,
                            PRUint32 *aState, PRUint32 *aExtraState,
                            eStateMapEntryID aStateMapEntryID);

private:
  
  nsIAtom** mAttributeName;

  
  PRBool mIsToken;

  
  PRUint32 mPermanentState;

  
  const char* mValue1;
  PRUint32 mState1;
  PRUint32 mExtraState1;

  const char* mValue2;
  PRUint32 mState2;
  PRUint32 mExtraState2;

  const char* mValue3;
  PRUint32 mState3;
  PRUint32 mExtraState3;

  
  PRUint32 mDefaultState;
  PRUint32 mDefaultExtraState;

  
  PRBool mDefinedIfAbsent;
};








struct nsRoleMapEntry
{
  
  const char *roleString;
  
  
  PRUint32 role;
  
  
  PRBool roleRule;
  
  
  EValueRule valueRule;

  
  EActionRule actionRule;

  
  
  ELiveAttrRule liveAttRule;

  
  PRUint32 state;   
  
  
  
  
  
  
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
};

#endif
