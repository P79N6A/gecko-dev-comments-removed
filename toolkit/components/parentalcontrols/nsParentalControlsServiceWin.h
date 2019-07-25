





































#ifndef nsParentalControlsServiceWin_h__
#define nsParentalControlsServiceWin_h__

#include "nsIParentalControlsService.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIURI.h"


#if (WINVER < 0x0600)
# undef WINVER
# define WINVER 0x0600
#endif

#include <wpcapi.h>
#include <wpcevent.h>

class nsParentalControlsServiceWin : public nsIParentalControlsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPARENTALCONTROLSSERVICE

  nsParentalControlsServiceWin();
  virtual ~nsParentalControlsServiceWin();

private:
  bool mEnabled;
  REGHANDLE mProvider;
  IWindowsParentalControls * mPC;

  void LogFileDownload(bool blocked, nsIURI *aSource, nsIFile *aTarget);
};

#endif 
