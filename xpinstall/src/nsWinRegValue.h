




































#ifndef nsWinRegValue_h__
#define nsWinRegValue_h__

#include "prtypes.h"


PR_BEGIN_EXTERN_C

struct nsWinRegValue {

public:

  
  PRInt32 type;
  void*   data;
  PRInt32 data_length;

  
  nsWinRegValue(PRInt32 datatype, void* regdata, PRInt32 len) 
	{
    type        = datatype;
    data        = regdata;
    data_length = len;
  } 
  
  
private:
  
  
  

  

};

PR_END_EXTERN_C

#endif
