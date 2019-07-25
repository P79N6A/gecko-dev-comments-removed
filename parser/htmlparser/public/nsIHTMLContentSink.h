



































#ifndef nsIHTMLContentSink_h___
#define nsIHTMLContentSink_h___










































#include "nsIParserNode.h"
#include "nsIContentSink.h"
#include "nsHTMLTags.h"


#define NS_IHTML_CONTENT_SINK_IID \
{ 0xd19e6730, 0x5e2f, 0x4131, \
  { 0x89, 0xdb, 0x8a, 0x91, 0x85, 0x15, 0x09, 0x7d } }

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

  







  NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode) = 0;

  





  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) = 0;

  




  NS_IMETHOD_(bool) IsFormOnStack() = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLContentSink, NS_IHTML_CONTENT_SINK_IID)

#endif 

