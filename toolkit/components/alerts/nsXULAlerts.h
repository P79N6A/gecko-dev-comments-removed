




#ifndef nsXULAlerts_h__
#define nsXULAlerts_h__

#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"

#include "nsIDOMWindow.h"
#include "nsIObserver.h"

using namespace mozilla;

class nsXULAlerts {
  friend class nsXULAlertObserver;
public:
  nsXULAlerts()
  {
    mNamedWindows.Init();
  }

  virtual ~nsXULAlerts() {}

  nsresult ShowAlertNotification(const nsAString& aImageUrl, const nsAString& aAlertTitle,
                                 const nsAString& aAlertText, bool aAlertTextClickable,
                                 const nsAString& aAlertCookie, nsIObserver* aAlertListener,
                                 const nsAString& aAlertName, const nsAString& aBidi,
                                 const nsAString& aLang);

  nsresult CloseAlert(const nsAString& aAlertName);
protected:
  nsInterfaceHashtable<nsStringHashKey, nsIDOMWindow> mNamedWindows;
};






class nsXULAlertObserver : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsXULAlertObserver(nsXULAlerts* aXULAlerts, const nsAString& aAlertName,
                     nsIObserver* aObserver)
    : mXULAlerts(aXULAlerts), mAlertName(aAlertName),
      mObserver(aObserver) {}

  void SetAlertWindow(nsIDOMWindow* aWindow) { mAlertWindow = aWindow; }

  virtual ~nsXULAlertObserver() {}
protected:
  nsXULAlerts* mXULAlerts;
  nsString mAlertName;
  nsCOMPtr<nsIDOMWindow> mAlertWindow;
  nsCOMPtr<nsIObserver> mObserver;
};

#endif 

