



#ifndef nsIHTMLContentSink_h___
#define nsIHTMLContentSink_h___













































#include "nsIParserNode.h"
#include "nsIContentSink.h"
#include "nsHTMLTags.h"

#define NS_IHTML_CONTENT_SINK_IID \
  {0xb08b0f29, 0xe61c, 0x4647, {0xaf, 0x1e, 0x05, 0x1a, 0x75, 0x2f, 0xe6, 0x3d}}





class nsIHTMLContentSink : public nsIContentSink 
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTML_CONTENT_SINK_IID)

  



  NS_IMETHOD IsEnabled(int32_t aTag, bool* aReturn) = 0;

    




     
  NS_IMETHOD OpenContainer(nsHTMLTag aNodeType) = 0;

  




     
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLContentSink, NS_IHTML_CONTENT_SINK_IID)

#endif 

