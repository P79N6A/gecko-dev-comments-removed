




































#ifndef nsAlertsIconListener_h__
#define nsAlertsIconListener_h__

#include "nsCOMPtr.h"
#include "imgIDecoderObserver.h"
#include "nsStringAPI.h"

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
                          const nsAString & aAlertText);

protected:
  nsCOMPtr<imgIRequest> mIconRequest;
  nsCString mAlertTitle;
  nsCString mAlertText;

  PRPackedBool mLoadedFrame;

  nsresult StartRequest(const nsAString & aImageUrl);
  nsresult ShowAlert(GdkPixbuf* aPixbuf);
};

#endif
