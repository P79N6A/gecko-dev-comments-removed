




































#ifndef nsIContentSink_h___
#define nsIContentSink_h___












#include "nsISupports.h"
#include "nsStringGlue.h"
#include "mozFlushType.h"

class nsIParser;

#define NS_ICONTENT_SINK_IID \
{ 0x6fd3c94f, 0xaf81, 0x4792, \
  { 0xa3, 0xe4, 0x1f, 0xb9, 0x40, 0xb6, 0x9c, 0x3a } }

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
  
  



  virtual PRBool IsScriptExecuting()
  {
    return PR_FALSE;
  }
  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentSink, NS_ICONTENT_SINK_IID)

#endif 
