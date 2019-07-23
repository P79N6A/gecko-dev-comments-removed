




































#ifndef nsIContentSink_h___
#define nsIContentSink_h___












#include "nsISupports.h"
#include "nsStringGlue.h"
#include "mozFlushType.h"

class nsIParser;

#define NS_ICONTENT_SINK_IID \
{ 0x94ec4df1, 0x6885, 0x4b1f, \
 { 0x85, 0x10, 0xe3, 0x5f, 0x4f, 0x36, 0xea, 0xaa } }

class nsIContentSink : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_SINK_IID)

  








  NS_IMETHOD WillTokenize(void)=0;

  




     
  NS_IMETHOD WillBuildModel(void)=0;

  




     
  NS_IMETHOD DidBuildModel()=0;

  





     
  NS_IMETHOD WillInterrupt(void)=0;

  




     
  NS_IMETHOD WillResume(void)=0;

  





  NS_IMETHOD SetParser(nsIParser* aParser)=0;

  





  virtual void FlushPendingNotifications(mozFlushType aType)=0;

  



  NS_IMETHOD SetDocumentCharset(nsACString& aCharset)=0;

  




  virtual nsISupports *GetTarget()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentSink, NS_ICONTENT_SINK_IID)

#endif 
