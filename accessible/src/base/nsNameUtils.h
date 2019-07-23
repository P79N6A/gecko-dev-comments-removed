






































#ifndef _nsNameUtils_H_
#define _nsNameUtils_H_

#include "prtypes.h"




enum ENameFromSubtreeRule
{
  
  eNoRule = 0x00,
  
  
  eFromSubtree = 0x01
};





class nsNameUtils
{
public:

  


  static PRUint32 gRoleToNameRulesMap[];
};

#endif
