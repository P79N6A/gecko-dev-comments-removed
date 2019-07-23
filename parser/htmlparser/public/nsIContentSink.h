




































#ifndef nsIContentSink_h___
#define nsIContentSink_h___












#include "nsISupports.h"
#include "nsStringGlue.h"
#include "mozFlushType.h"
#include "nsIDTD.h"

class nsIParser;


#define NS_ICONTENT_SINK_IID \
{ 0x5530ebaf, 0xf9fd, 0x44bf, \
  { 0xb6, 0xb5, 0xe4, 0x6f, 0x3b, 0x67, 0xeb, 0x3d } }

class nsIContentSink : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_SINK_IID)

  





  NS_IMETHOD WillParse(void)=0;

  








  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) {
    return NS_OK;
  }

  








  NS_IMETHOD DidBuildModel() {
    return NS_OK;
  }

  





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
