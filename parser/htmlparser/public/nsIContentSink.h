




































#ifndef nsIContentSink_h___
#define nsIContentSink_h___












#include "nsISupports.h"
#include "nsStringGlue.h"
#include "mozFlushType.h"

class nsIParser;

#define NS_ICONTENT_SINK_IID \
{ 0xcfa3643b, 0xee60, 0x4bf0, \
  { 0xbc, 0x83, 0x49, 0x95, 0xdb, 0xbc, 0xda, 0x75 } }

class nsIContentSink : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_SINK_IID)

  





  NS_IMETHOD WillParse(void)=0;

  





  NS_IMETHOD WillBuildModel(void)=0;

  





  NS_IMETHOD DidBuildModel()=0;

  





  virtual PRBool ReadyToCallDidBuildModel(PRBool aTerminated)
  {
    return PR_TRUE;
  };

  






  NS_IMETHOD WillInterrupt(void)=0;

  





  NS_IMETHOD WillResume(void)=0;

  





  NS_IMETHOD SetParser(nsIParser* aParser)=0;

  





  virtual void FlushPendingNotifications(mozFlushType aType)=0;

  



  NS_IMETHOD SetDocumentCharset(nsACString& aCharset)=0;

  




  virtual nsISupports *GetTarget()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentSink, NS_ICONTENT_SINK_IID)

#endif 
