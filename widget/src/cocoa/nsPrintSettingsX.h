




































#ifndef nsPrintSettingsX_h_
#define nsPrintSettingsX_h_

#include "nsPrintSettingsImpl.h"  

#ifdef MOZ_COCOA_PRINTING
#import <Cocoa/Cocoa.h>
#else
#include "nsIPrintSettingsX.h"  
#endif

class nsPrintSettingsX : public nsPrintSettings
#ifndef MOZ_COCOA_PRINTING
                       , public nsIPrintSettingsX
#endif
{
public:
  NS_DECL_ISUPPORTS_INHERITED
#ifndef MOZ_COCOA_PRINTING
  NS_DECL_NSIPRINTSETTINGSX
#endif

  nsPrintSettingsX();
  virtual ~nsPrintSettingsX();
  nsresult Init();

#ifdef MOZ_COCOA_PRINTING
  NSPrintInfo* GetCocoaPrintInfo() { return mPrintInfo; }
  virtual nsresult ReadPageFormatFromPrefs();
  virtual nsresult WritePageFormatToPrefs();
#endif

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
#ifdef MOZ_COCOA_PRINTING
  NSPrintInfo* mPrintInfo;
#endif
};

#endif 
