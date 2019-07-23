






































#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

#include "prtypes.h"
#include "nsAccessibilityAtoms.h"


enum ENameRule
{
  
  
  
  
  
  eNameLabelOrTitle,
  
  
  
  
  
  
  
  
  eNameOkFromChildren
};


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
  eJumpAction,
  eOpenCloseAction,
  eSelectAction,
  eSwitchAction
};


#define kBoolState 0


#define kNoReqStates 0



struct nsStateMapEntry
{
  nsIAtom** attributeName;  
  const char* attributeValue; 
  PRUint32 state;             
};


struct nsRoleMapEntry
{
  
  const char *roleString;
  
  
  PRUint32 role;
  
  
  ENameRule nameRule;
  
  
  EValueRule valueRule;

  
  EActionRule actionRule;

  
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
};

#endif
