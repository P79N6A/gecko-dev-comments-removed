



#ifndef nsIContentSink_h___
#define nsIContentSink_h___












#include "nsISupports.h"
#include "nsString.h"
#include "mozFlushType.h"
#include "nsIDTD.h"

class nsParserBase;

#define NS_ICONTENT_SINK_IID \
{ 0xcf9a7cbb, 0xfcbc, 0x4e13, \
  { 0x8e, 0xf5, 0x18, 0xef, 0x2d, 0x3d, 0x58, 0x29 } }

class nsIContentSink : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENT_SINK_IID)

  





  NS_IMETHOD WillParse(void)=0;

  








  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) {
    return NS_OK;
  }

  








  NS_IMETHOD DidBuildModel(bool aTerminated) {
    return NS_OK;
  }

  






  NS_IMETHOD WillInterrupt(void)=0;

  





  NS_IMETHOD WillResume(void)=0;

  





  NS_IMETHOD SetParser(nsParserBase* aParser)=0;

  





  virtual void FlushPendingNotifications(mozFlushType aType)=0;

  



  NS_IMETHOD SetDocumentCharset(nsACString& aCharset)=0;

  




  virtual nsISupports *GetTarget()=0;
  
  



  virtual bool IsScriptExecuting()
  {
    return false;
  }
  
  


  virtual void ContinueInterruptedParsingAsync() {}
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentSink, NS_ICONTENT_SINK_IID)

#endif 
