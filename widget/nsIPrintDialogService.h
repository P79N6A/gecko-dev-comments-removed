




































#ifndef nsIPrintDialogService_h__
#define nsIPrintDialogService_h__

#include "nsISupports.h"

class nsIDOMWindow;
class nsIPrintSettings;
class nsIWebBrowserPrint;





#define NS_IPRINTDIALOGSERVICE_IID \
{ 0x3715eb1a, 0xb314, 0x447c, \
{ 0x95, 0x33, 0xd0, 0x6a, 0x6d, 0xa6, 0xa6, 0xf0 } }





class nsIPrintDialogService  : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRINTDIALOGSERVICE_IID)

  



  NS_IMETHOD Init() = 0;

  












  NS_IMETHOD Show(nsIDOMWindow *aParent, nsIPrintSettings *aSettings,
                  nsIWebBrowserPrint *aWebBrowserPrint) = 0;

  









  NS_IMETHOD ShowPageSetup(nsIDOMWindow *aParent, nsIPrintSettings *aSettings) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrintDialogService, NS_IPRINTDIALOGSERVICE_IID)

#define NS_PRINTDIALOGSERVICE_CONTRACTID ("@mozilla.org/widget/printdialog-service;1")

#endif 

