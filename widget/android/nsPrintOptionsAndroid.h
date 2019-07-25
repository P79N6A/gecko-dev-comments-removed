




































#ifndef nsPrintOptionsAndroid_h__
#define nsPrintOptionsAndroid_h__

#include "nsPrintOptionsImpl.h"
#include "nsIPrintSettings.h"




class nsPrintOptionsAndroid : public nsPrintOptions
{
public:
  nsPrintOptionsAndroid();
  virtual ~nsPrintOptionsAndroid();

  NS_IMETHOD CreatePrintSettings(nsIPrintSettings **_retval);
};

#endif 
