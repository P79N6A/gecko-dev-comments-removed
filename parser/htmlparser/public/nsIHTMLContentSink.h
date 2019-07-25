



































#ifndef nsIHTMLContentSink_h___
#define nsIHTMLContentSink_h___













































#include "nsIParserNode.h"
#include "nsIContentSink.h"
#include "nsHTMLTags.h"

#define NS_IHTML_CONTENT_SINK_IID \
{ 0xb6d6ae00, 0x0884, 0x4a30, \
  { 0xa8, 0xb4, 0xce, 0xca, 0x57, 0x27, 0x1a, 0x3e } }

#define MAX_REFLOW_DEPTH  200





class nsIHTMLContentSink : public nsIContentSink 
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTML_CONTENT_SINK_IID)

  




  NS_IMETHOD OpenHead() = 0;

  






  NS_IMETHOD BeginContext(PRInt32 aPosition) = 0;
  
  






  NS_IMETHOD EndContext(PRInt32 aPosition) = 0;
  
  



  NS_IMETHOD IsEnabled(PRInt32 aTag, bool* aReturn) = 0;

  





  NS_IMETHOD DidProcessTokens() = 0;

  



  NS_IMETHOD WillProcessAToken(void) = 0;

  





  NS_IMETHOD DidProcessAToken(void) = 0;

    




     
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode) = 0;

  




     
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag) = 0;

  






  NS_IMETHOD CloseMalformedContainer(const nsHTMLTag aTag)
  {
    return CloseContainer(aTag);
  }

  






     
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLContentSink, NS_IHTML_CONTENT_SINK_IID)

#endif 

