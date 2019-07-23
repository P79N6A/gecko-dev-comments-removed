





































#include "nsServiceManagerUtils.h"
#include "nsIParserService.h"
#include "nsEditorParserObserver.h"

NS_IMPL_ADDREF(nsEditorParserObserver)
NS_IMPL_RELEASE(nsEditorParserObserver)

NS_INTERFACE_MAP_BEGIN(nsEditorParserObserver)
      NS_INTERFACE_MAP_ENTRY(nsIElementObserver)
      NS_INTERFACE_MAP_ENTRY(nsIObserver)
      NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
      NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIElementObserver)
NS_INTERFACE_MAP_END

nsEditorParserObserver::nsEditorParserObserver()
: mBadTagFound(PR_FALSE)
{
}

nsEditorParserObserver::~nsEditorParserObserver()
{
}

NS_IMETHODIMP nsEditorParserObserver::Notify(
                     PRUint32 aDocumentID, 
                     const PRUnichar* aTag, 
                     PRUint32 numOfAttributes, 
                     const PRUnichar* nameArray[], 
                     const PRUnichar* valueArray[])
{
  
  Notify();
  return NS_OK;
}

NS_IMETHODIMP nsEditorParserObserver::Notify(
                     PRUint32 aDocumentID, 
                     eHTMLTags aTag, 
                     PRUint32 numOfAttributes, 
                     const PRUnichar* nameArray[], 
                     const PRUnichar* valueArray[])
{
  if (eHTMLTag_frameset == aTag)
  {
    Notify();
    return NS_OK;
  }
  else
    return NS_ERROR_ILLEGAL_VALUE;
}
NS_IMETHODIMP nsEditorParserObserver::Notify(nsISupports* aWebShell, 
                                             nsISupports* aChannel, 
                                             const PRUnichar* aTag, 
                                             const nsStringArray* aKeys, 
                                             const nsStringArray* aValues,
                                             const PRUint32 aFlags)
{
  Notify();
  return NS_OK;
}

NS_IMETHODIMP nsEditorParserObserver::Observe(nsISupports*, const char*, const PRUnichar*)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

void nsEditorParserObserver::Notify()
{
  mBadTagFound = PR_TRUE;
}

NS_IMETHODIMP nsEditorParserObserver::Start(eHTMLTags* aWatchTags) 
{
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIParserService> parserService(do_GetService("@mozilla.org/parser/parser-service;1"));
    
  if (!parserService) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  res = parserService->RegisterObserver(this,
                                        NS_LITERAL_STRING("text/html"),
                                        aWatchTags);
  return res;
}

NS_IMETHODIMP nsEditorParserObserver::End() 
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIParserService> parserService(do_GetService("@mozilla.org/parser/parser-service;1"));

  if (!parserService) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
   
  res = parserService->UnregisterObserver(this,
                                          NS_LITERAL_STRING("text/html"));
  return res;
}

NS_IMETHODIMP nsEditorParserObserver::GetBadTagFound(PRBool *aFound)
{
  NS_ENSURE_ARG_POINTER(aFound);
  *aFound = mBadTagFound;
  return NS_OK; 
}



