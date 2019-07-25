




































#ifndef nsIContentSink_h___
#define nsIContentSink_h___












#include "nsISupports.h"
#include "nsStringGlue.h"
#include "mozFlushType.h"
#include "nsIDTD.h"

class nsIParser;

#define NS_ICONTENT_SINK_IID \
{ 0x46983927, 0x7ff5, 0x42cc, \
  { 0x9f, 0x57, 0xf2, 0xd6, 0xa8, 0x42, 0x52, 0x18 } }

class nsIContentSink : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_SINK_IID)

  





  NS_IMETHOD WillParse(void)=0;

  








  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) {
    return NS_OK;
  }

  








  NS_IMETHOD DidBuildModel(PRBool aTerminated) {
    return NS_OK;
  }

  






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

  



  virtual void ScrollToRef() = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentSink, NS_ICONTENT_SINK_IID)

#endif 
