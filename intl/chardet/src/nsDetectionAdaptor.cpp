





































#include "nsString.h"
#include "plstr.h"
#include "pratom.h"
#include "nsCharDetDll.h"
#include "nsIParser.h"
#include "nsIDocument.h"
#include "nsDetectionAdaptor.h"
#include "nsIContentSink.h"



NS_IMETHODIMP nsMyObserver::Notify(
    const char* aCharset, nsDetectionConfident aConf)
{
    nsresult rv = NS_OK;

    if(mWeakRefParser) {
      nsCAutoString existingCharset;
      PRInt32 existingSource;
      mWeakRefParser->GetDocumentCharset(existingCharset, existingSource);  
      if (existingSource >= kCharsetFromAutoDetection) 
        return NS_OK;
    }
     
    if(!mCharset.Equals(aCharset)) {
      if(mNotifyByReload) {
        rv = mWebShellSvc->SetRendering( PR_FALSE);
        rv = mWebShellSvc->StopDocumentLoad();
        rv = mWebShellSvc->ReloadDocument(aCharset, kCharsetFromAutoDetection);
      } else {
        nsDependentCString newcharset(aCharset);
        if (mWeakRefParser) {
          mWeakRefParser->SetDocumentCharset(newcharset, kCharsetFromAutoDetection);
          nsCOMPtr<nsIContentSink> contentSink = mWeakRefParser->GetContentSink();
          if (contentSink)
            contentSink->SetDocumentCharset(newcharset);
        }
        if(mWeakRefDocument) 
          mWeakRefDocument->SetDocumentCharacterSet(newcharset);
      }
    }
    return NS_OK;
}

NS_IMETHODIMP nsMyObserver::Init( nsIWebShellServices* aWebShellSvc, 
                   nsIDocument* aDocument,
                   nsIParser* aParser,
                   const char* aCharset,
                   const char* aCommand)
{
    if(aCommand) {
        mCommand = aCommand;
    }
    if(aCharset) {
        mCharset = aCharset;
    }
    if(aDocument) {
        mWeakRefDocument = aDocument;
    }
    if(aParser) {
        mWeakRefParser = aParser;
    }
    if(nsnull != aWebShellSvc)
    {
        mWebShellSvc = aWebShellSvc;
        return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMPL_ISUPPORTS1 ( nsMyObserver ,nsICharsetDetectionObserver)


nsDetectionAdaptor::nsDetectionAdaptor( void ) 
{
     mDontFeedToDetector = PR_TRUE;
}

nsDetectionAdaptor::~nsDetectionAdaptor()
{
}


NS_IMPL_ISUPPORTS2 (nsDetectionAdaptor, nsIParserFilter, nsICharsetDetectionAdaptor)


NS_IMETHODIMP nsDetectionAdaptor::Init(
    nsIWebShellServices* aWebShellSvc, nsICharsetDetector *aDetector,
    nsIDocument* aDocument, nsIParser* aParser, const char* aCharset,
    const char* aCommand)
{
  if((nsnull != aWebShellSvc) && (nsnull != aDetector) && (nsnull != aCharset))
  {
    nsresult rv = NS_OK;
    mObserver = new nsMyObserver();
    if(!mObserver)
       return NS_ERROR_OUT_OF_MEMORY;

    rv = mObserver->Init(aWebShellSvc, aDocument, aParser, aCharset, aCommand);
    if(NS_SUCCEEDED(rv)) {
      rv = aDetector->Init(mObserver.get());
      if(NS_SUCCEEDED(rv)) {
        mDetector = aDetector;
        mDontFeedToDetector = PR_FALSE;
        return NS_OK;
      }
    }
  }
  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP nsDetectionAdaptor::RawBuffer
    (const char * buffer, PRUint32 * buffer_length) 
{
    if((mDontFeedToDetector) || (!mDetector))
       return NS_OK;
    nsresult rv = NS_OK;
    rv = mDetector->DoIt((const char*)buffer, *buffer_length, &mDontFeedToDetector);
    if(mObserver) 
       mObserver->SetNotifyByReload(PR_TRUE);

    return NS_OK;
}

NS_IMETHODIMP nsDetectionAdaptor::Finish()
{
    if((mDontFeedToDetector) || (!mDetector))
       return NS_OK;
    nsresult rv = NS_OK;
    rv = mDetector->Done();

    return NS_OK;
}

