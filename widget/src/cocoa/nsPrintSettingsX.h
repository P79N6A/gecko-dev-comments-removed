




































#ifndef nsPrintSettingsX_h_
#define nsPrintSettingsX_h_

#include "nsPrintSettingsImpl.h"  
#include "nsIPrintSettingsX.h"  

class nsPrintSettingsX : public nsPrintSettings,
                         public nsIPrintSettingsX
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPRINTSETTINGSX

  nsPrintSettingsX();
  virtual ~nsPrintSettingsX();

  nsresult Init();

protected:
  nsPrintSettingsX(const nsPrintSettingsX& src);
  nsPrintSettingsX& operator=(const nsPrintSettingsX& rhs);

  nsresult _Clone(nsIPrintSettings **_retval);
  nsresult _Assign(nsIPrintSettings *aPS);

  
  
  nsresult InitUnwriteableMargin();

  
  OSStatus CreateDefaultPageFormat(PMPrintSession aSession, PMPageFormat& outFormat);
  OSStatus CreateDefaultPrintSettings(PMPrintSession aSession, PMPrintSettings& outSettings);

  PMPageFormat mPageFormat;
  PMPrintSettings mPrintSettings;
};

#endif 
