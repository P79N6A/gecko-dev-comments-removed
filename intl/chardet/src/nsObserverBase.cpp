






































#include "nsIServiceManager.h"
#include "nsIWebShellServices.h"
#include "nsObserverBase.h"
#include "nsString.h"
#include "nsIHttpChannel.h"




NS_IMETHODIMP nsObserverBase::NotifyWebShell(nsISupports* aWebShell,
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
   wss = do_QueryInterface(aWebShell,&res);
   if (NS_SUCCEEDED(res)) {

#ifndef DONT_INFORM_WEBSHELL
     
     if (NS_FAILED( res = wss->SetRendering(PR_FALSE) ))
       rv = res;

     

     else if (NS_FAILED(res = wss->StopDocumentLoad())){
             rv = wss->SetRendering(PR_TRUE); 
     }
     else if (NS_FAILED(res = wss->ReloadDocument(charset, source))) {
             rv = wss->SetRendering(PR_TRUE); 
     }
     else {
       rv = NS_ERROR_HTMLPARSER_STOPPARSING; 
     }
#endif
   }

   
  if (rv != NS_ERROR_HTMLPARSER_STOPPARSING) 
    rv = NS_ERROR_HTMLPARSER_CONTINUE;

  return rv;
}
