




































#ifndef nsSAXXMLReader_h__
#define nsSAXXMLReader_h__

#include "nsCOMPtr.h"
#include "nsIContentSink.h"
#include "nsIExtendedExpatSink.h"
#include "nsIParser.h"
#include "nsIURI.h"
#include "nsISAXXMLReader.h"
#include "nsISAXContentHandler.h"
#include "nsISAXDTDHandler.h"
#include "nsISAXErrorHandler.h"
#include "nsISAXLexicalHandler.h"
#include "nsCycleCollectionParticipant.h"

#define NS_SAXXMLREADER_CONTRACTID "@mozilla.org/saxparser/xmlreader;1"
#define NS_SAXXMLREADER_CLASSNAME "SAX XML Reader"
#define NS_SAXXMLREADER_CID  \
{ 0x1ab57b84, 0xb4dd, 0x4a03, \
{ 0x8f, 0xa9, 0x80, 0xd9, 0x24, 0x62, 0xf9, 0x49 } }

class nsSAXXMLReader : public nsISAXXMLReader,
                       public nsIExtendedExpatSink,
                       public nsIContentSink
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsSAXXMLReader, nsISAXXMLReader)
  NS_DECL_NSIEXPATSINK
  NS_DECL_NSIEXTENDEDEXPATSINK
  NS_DECL_NSISAXXMLREADER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsSAXXMLReader();

  
  NS_IMETHOD WillParse()
  {
    return NS_OK;
  }

  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
  NS_IMETHOD DidBuildModel(PRBool aTerminated);
  NS_IMETHOD SetParser(nsIParser* aParser);
  
  NS_IMETHOD WillInterrupt()
  {
    return NS_OK;
  }

  NS_IMETHOD WillResume()
  {
    return NS_OK;
  }
  
  virtual void FlushPendingNotifications(mozFlushType aType)
  {
  }
  
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset)
  {
    return NS_OK;
  }
  
  virtual nsISupports *GetTarget()
  {
    return nsnull;
  }

  virtual void ScrollToRef()
  {
  }

private:
  nsCOMPtr<nsISAXContentHandler> mContentHandler;
  nsCOMPtr<nsISAXDTDHandler> mDTDHandler;
  nsCOMPtr<nsISAXErrorHandler> mErrorHandler;
  nsCOMPtr<nsISAXLexicalHandler> mLexicalHandler;
  nsCOMPtr<nsIURI> mBaseURI;
  nsCOMPtr<nsIStreamListener> mListener;
  nsCOMPtr<nsIRequestObserver> mParserObserver;
  PRBool mIsAsyncParse;
  static PRBool TryChannelCharset(nsIChannel *aChannel,
                                  PRInt32& aCharsetSource,
                                  nsACString& aCharset);
  nsresult EnsureBaseURI();
  nsresult InitParser(nsIRequestObserver *aListener, nsIChannel *aChannel);
  nsresult SplitExpatName(const PRUnichar *aExpatName,
                          nsString &aURI,
                          nsString &aLocalName,
                          nsString &aQName);
  nsString mPublicId;
  nsString mSystemId;
};

#endif 
