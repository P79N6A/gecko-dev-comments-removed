





#ifndef nsPrintOptionsGTK_h__
#define nsPrintOptionsGTK_h__

#include "nsPrintOptionsImpl.h"  





class nsPrintOptionsGTK : public nsPrintOptions
{
public:
  nsPrintOptionsGTK();
  virtual ~nsPrintOptionsGTK();

  virtual nsresult _CreatePrintSettings(nsIPrintSettings **_retval);

};



#endif 
