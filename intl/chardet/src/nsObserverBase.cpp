




































#include "nsIServiceManager.h"
#include "nsIWebShellServices.h"
#include "nsObserverBase.h"
#include "nsString.h"
#include "nsIHttpChannel.h"




NS_IMETHODIMP nsObserverBase::NotifyDocShell(nsISupports* aDocShell,
                                             nsISupports* aChannel,
                                             const char* charset, 
                                             PRInt32 source)
{

   nsresult rv  = NS_OK;
   nsresult res = NS_OK;

   nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel,&res));
   if (NS_SUCCEEDED(res)) {
     nsCAutoString method;
     httpChannel->GetRequestMethod(method);
     if (method.EqualsLiteral("POST")) { 
       return NS_OK;
     }
   }

   nsCOMPtr<nsIWebShellServices> wss;
   wss = do_QueryInterface(aDocShell,&res);
   if (NS_SUCCEEDED(res)) {
     
     if (NS_FAILED(wss->StopDocumentLoad())){
       
     }
     else if (NS_FAILED(wss->ReloadDocument(charset, source))) {
       
     }
     else {
       rv = NS_ERROR_HTMLPARSER_STOPPARSING; 
     }
   }

   
  if (rv != NS_ERROR_HTMLPARSER_STOPPARSING) 
    rv = NS_ERROR_HTMLPARSER_CONTINUE;

  return rv;
}
