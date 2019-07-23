



































#ifndef nsDetectionAdaptor_h__
#define nsDetectionAdaptor_h__

#include "nsCOMPtr.h"
#include "nsIWebShellServices.h"

#include "nsIParserFilter.h"
static NS_DEFINE_IID(kIParserFilterIID, NS_IPARSERFILTER_IID);

class nsIDocument;
class CToken;


class nsMyObserver : public nsICharsetDetectionObserver
{
 public:
   NS_DECL_ISUPPORTS

 public:
   nsMyObserver( void )
   {
     mWebShellSvc = nsnull;
     mNotifyByReload = PR_FALSE;
     mWeakRefDocument = nsnull;
     mWeakRefParser = nsnull;
   }
   virtual  ~nsMyObserver( void )
   {
     
     
   }


   
   NS_IMETHOD Init(nsIWebShellServices* aWebShellSvc, 
                   nsIDocument* aDocument,
                   nsIParser* aParser,
                   const char* aCharset,
                   const char* aCommand);

   
   NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf);
   void SetNotifyByReload(PRBool aByReload) { mNotifyByReload = aByReload; }
 private:
     nsCOMPtr<nsIWebShellServices> mWebShellSvc;
     PRBool mNotifyByReload;

     
     
     
     
     
     
     
     
     nsIDocument* mWeakRefDocument;
     nsIParser* mWeakRefParser;
     nsCAutoString mCharset;
     nsCAutoString mCommand;
};

class nsDetectionAdaptor : 
                           public nsIParserFilter,
                           public nsICharsetDetectionAdaptor
{
 public:
   NS_DECL_ISUPPORTS

 public:
   nsDetectionAdaptor( void );
   virtual  ~nsDetectionAdaptor( void );

   
   NS_IMETHOD Init(nsIWebShellServices* aWebShellSvc, nsICharsetDetector *aDetector, 
                   nsIDocument* aDocument,
                   nsIParser* aParser,
                   const char* aCharset,
                   const char* aCommand=nsnull);
  
   
   NS_IMETHOD RawBuffer(const char * buffer, PRUint32 * buffer_length) ;
   NS_IMETHOD Finish();

   
   
   NS_IMETHOD WillAddToken(CToken & token) { return NS_OK; }
   NS_IMETHOD ProcessTokens( void ) {return NS_OK;}

  private:
     nsCOMPtr<nsICharsetDetector> mDetector;
     PRBool mDontFeedToDetector;
     nsCOMPtr<nsMyObserver> mObserver; 
};

#endif 
