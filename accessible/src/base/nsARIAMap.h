






































#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

#include "prtypes.h"


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


#define kBoolState 0


#define kNoReqStates 0



struct nsStateMapEntry
{
  const char* attributeName;  
  const char* attributeValue; 
  PRUint32 state;             
};


struct nsRoleMapEntry
{
  
  const char *roleString;
  
  
  PRUint32 role;
  
  
  ENameRule nameRule;
  
  
  EValueRule valueRule;
  
  
  PRUint32 state;   
  
  
  
  
  
  
  nsStateMapEntry attributeMap1;
  nsStateMapEntry attributeMap2;
  nsStateMapEntry attributeMap3;
  nsStateMapEntry attributeMap4;
  nsStateMapEntry attributeMap5;
  nsStateMapEntry attributeMap6;
  nsStateMapEntry attributeMap7;
};






struct nsARIAMap
{
  static nsRoleMapEntry gWAIRoleMap[];
  static nsStateMapEntry gWAIUnivStateMap[];
};

#endif
