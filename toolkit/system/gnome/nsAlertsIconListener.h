




































#ifndef nsAlertsIconListener_h__
#define nsAlertsIconListener_h__

#include "nsCOMPtr.h"
#include "imgIDecoderObserver.h"
#include "nsStringAPI.h"
#include "nsIObserver.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

class imgIRequest;

class nsAlertsIconListener : public imgIDecoderObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_IMGIDECODEROBSERVER

  nsAlertsIconListener();
  virtual ~nsAlertsIconListener();

  nsresult InitAlertAsync(const nsAString & aImageUrl,
                          const nsAString & aAlertTitle, 
                          const nsAString & aAlertText,
                          PRBool aAlertTextClickable,
                          const nsAString & aAlertCookie,
                          nsIObserver * aAlertListener);

  void SendCallback();
  void SendClosed();

protected:
  nsCOMPtr<imgIRequest> mIconRequest;
  nsCString mAlertTitle;
  nsCString mAlertText;

  nsCOMPtr<nsIObserver> mAlertListener;
  nsString mAlertCookie;

  PRPackedBool mLoadedFrame;
  PRPackedBool mAlertHasAction;

  nsresult StartRequest(const nsAString & aImageUrl);
  nsresult ShowAlert(GdkPixbuf* aPixbuf);
};

#endif
