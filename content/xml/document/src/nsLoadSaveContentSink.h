





































#ifndef nsLoadSaveContentSink_h__
#define nsLoadSaveContentSink_h__

#include "nsIExpatSink.h"
#include "nsIXMLContentSink.h"
#include "nsILoadSaveContentSink.h"
#include "nsCOMPtr.h"






class nsLoadSaveContentSink : public nsILoadSaveContentSink,
                              public nsIExpatSink
{
public:
  nsLoadSaveContentSink();
  virtual ~nsLoadSaveContentSink();

  






  nsresult Init(nsIXMLContentSink* aBaseSink);
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEXPATSINK

  
  NS_IMETHOD WillBuildModel(void);
  NS_IMETHOD DidBuildModel(void);
  NS_IMETHOD WillInterrupt(void);
  NS_IMETHOD WillResume(void);
  NS_IMETHOD SetParser(nsIParser* aParser);  
  virtual void FlushPendingNotifications(mozFlushType aType);
  NS_IMETHOD SetDocumentCharset(nsAString& aCharset);

private:
  



  nsCOMPtr<nsIXMLContentSink> mBaseSink;
  nsCOMPtr<nsIExpatSink> mExpatSink;
};

#endif 
