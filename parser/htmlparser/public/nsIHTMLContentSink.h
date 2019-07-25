



































#ifndef nsIHTMLContentSink_h___
#define nsIHTMLContentSink_h___










































#include "nsIParserNode.h"
#include "nsIContentSink.h"
#include "nsHTMLTags.h"

#define NS_IHTML_CONTENT_SINK_IID \
{ 0xa3aad227, 0xe137, 0x407c, \
  { 0xa4, 0xa0, 0x9e, 0x23, 0xb6, 0x38, 0xf3, 0x42 } }

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

  






     
  NS_IMETHOD AddComment(const nsIParserNode& aNode) = 0;

  






     
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode) = 0;

  




  NS_IMETHOD_(bool) IsFormOnStack() = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLContentSink, NS_IHTML_CONTENT_SINK_IID)

#endif 

