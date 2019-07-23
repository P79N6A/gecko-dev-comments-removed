



































#ifndef nsIHTMLContentSink_h___
#define nsIHTMLContentSink_h___










































#include "nsIParserNode.h"
#include "nsIContentSink.h"
#include "nsHTMLTags.h"

#define NS_IHTML_CONTENT_SINK_IID \
{ 0x73b5a072, 0x0f87, 0x4d07, \
  { 0xa8, 0x16, 0xe6, 0xac, 0x73, 0xa7, 0x04, 0x3c } }


#if defined(XP_MAC) || defined(WINCE) 
#define MAX_REFLOW_DEPTH  75    //setting to 75 to prevent layout from crashing on mac. Bug 55095.
                                
                                
#else
#define MAX_REFLOW_DEPTH  200   //windows and linux (etc) can do much deeper structures.
#endif

class nsIHTMLContentSink : public nsIContentSink 
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTML_CONTENT_SINK_IID)

  




  NS_IMETHOD OpenHead() = 0;

  






  NS_IMETHOD BeginContext(PRInt32 aPosition) = 0;
  
  






  NS_IMETHOD EndContext(PRInt32 aPosition) = 0;
  
  



  NS_IMETHOD IsEnabled(PRInt32 aTag, PRBool* aReturn) = 0;

   



  NS_IMETHOD WillProcessTokens(void) = 0;

  





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

  




  NS_IMETHOD_(PRBool) IsFormOnStack() = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLContentSink, NS_IHTML_CONTENT_SINK_IID)

#endif 

