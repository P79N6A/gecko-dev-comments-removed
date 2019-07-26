




#ifndef nsParentalControlsService_h__
#define nsParentalControlsService_h__

#include "nsIParentalControlsService.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIURI.h"

#if defined(XP_WIN)

#if (WINVER < 0x0600)
# undef WINVER
# define WINVER 0x0600
#endif
#include <wpcapi.h>
#include <wpcevent.h>
#endif

class nsParentalControlsService : public nsIParentalControlsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPARENTALCONTROLSSERVICE

  nsParentalControlsService();
  virtual ~nsParentalControlsService();

private:
  bool mEnabled;
#if defined(XP_WIN)
  REGHANDLE mProvider;
  IWindowsParentalControls * mPC;
  void LogFileDownload(bool blocked, nsIURI *aSource, nsIFile *aTarget);
#endif
};

#endif 
