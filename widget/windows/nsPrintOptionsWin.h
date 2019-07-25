






































#ifndef nsPrintOptionsWin_h__
#define nsPrintOptionsWin_h__

#include "nsPrintOptionsImpl.h"  





class nsPrintOptionsWin : public nsPrintOptions
{
public:
  nsPrintOptionsWin();
  virtual ~nsPrintOptionsWin();

  virtual nsresult _CreatePrintSettings(nsIPrintSettings **_retval);
};



#endif 
