




#ifndef nsPrintSettingsX_h_
#define nsPrintSettingsX_h_

#include "nsPrintSettingsImpl.h"  
#import <Cocoa/Cocoa.h>

#define NS_PRINTSETTINGSX_IID \
{ 0x0DF2FDBD, 0x906D, 0x4726, \
  { 0x9E, 0x4D, 0xCF, 0xE0, 0x87, 0x8D, 0x70, 0x7C } }

class nsPrintSettingsX : public nsPrintSettings
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PRINTSETTINGSX_IID)
  NS_DECL_ISUPPORTS_INHERITED

  nsPrintSettingsX();
  nsresult Init();
  NSPrintInfo* GetCocoaPrintInfo() { return mPrintInfo; }
  void SetCocoaPrintInfo(NSPrintInfo* aPrintInfo);
  virtual nsresult ReadPageFormatFromPrefs();
  virtual nsresult WritePageFormatToPrefs();

  PMPrintSettings GetPMPrintSettings();
  PMPrintSession GetPMPrintSession();
  PMPageFormat GetPMPageFormat();
  void SetPMPageFormat(PMPageFormat aPageFormat);

protected:
  virtual ~nsPrintSettingsX();

  nsPrintSettingsX(const nsPrintSettingsX& src);
  nsPrintSettingsX& operator=(const nsPrintSettingsX& rhs);

  nsresult _Clone(nsIPrintSettings **_retval);
  nsresult _Assign(nsIPrintSettings *aPS);

  
  
  nsresult InitUnwriteableMargin();

  
  OSStatus CreateDefaultPageFormat(PMPrintSession aSession, PMPageFormat& outFormat);
  OSStatus CreateDefaultPrintSettings(PMPrintSession aSession, PMPrintSettings& outSettings);

  NSPrintInfo* mPrintInfo;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPrintSettingsX, NS_PRINTSETTINGSX_IID)

#endif 
