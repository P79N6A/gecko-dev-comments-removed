





#ifndef nsPrintSettingsWin_h__
#define nsPrintSettingsWin_h__

#include "nsPrintSettingsImpl.h"  
#include "nsIPrintSettingsWin.h"  
#include <windows.h>  





class nsPrintSettingsWin : public nsPrintSettings,
                           public nsIPrintSettingsWin
{
  virtual ~nsPrintSettingsWin();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPRINTSETTINGSWIN

  nsPrintSettingsWin();
  nsPrintSettingsWin(const nsPrintSettingsWin& aPS);

  


  virtual nsresult _Clone(nsIPrintSettings **_retval);

  


  virtual nsresult _Assign(nsIPrintSettings* aPS);

  


  nsPrintSettingsWin& operator=(const nsPrintSettingsWin& rhs);

protected:
  void CopyDevMode(DEVMODEW* aInDevMode, DEVMODEW *& aOutDevMode);

  wchar_t*      mDeviceName;
  wchar_t*      mDriverName;
  LPDEVMODEW mDevMode;
};



#endif 
