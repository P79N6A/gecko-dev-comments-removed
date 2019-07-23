






































#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

#include "prtypes.h"
#include "nsAccessibilityAtoms.h"


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


#define kBoolState 0


#define kNoReqStates 0



struct nsStateMapEntry
{
  nsIAtom** attributeName;  
  const char* attributeValue; 
  PRUint32 state;             
};


struct nsAttributeCharacteristics
{
  nsIAtom** attributeName;
  const PRUint8 characteristics;
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
  
  
  
  
  
  
  nsStateMapEntry attributeMap1;
  nsStateMapEntry attributeMap2;
  nsStateMapEntry attributeMap3;
  nsStateMapEntry attributeMap4;
  nsStateMapEntry attributeMap5;
  nsStateMapEntry attributeMap6;
  nsStateMapEntry attributeMap7;
  nsStateMapEntry attributeMap8;
};






struct nsARIAMap
{
  


  static nsRoleMapEntry gWAIRoleMap[];
  static PRUint32 gWAIRoleMapLength;

  



  static nsRoleMapEntry gLandmarkRoleMap;

  




  static nsRoleMapEntry gEmptyRoleMap;

  



  static nsStateMapEntry gWAIUnivStateMap[];
  
  


  static nsAttributeCharacteristics gWAIUnivAttrMap[];
  static PRUint32 gWAIUnivAttrMapLength;
};

#endif
