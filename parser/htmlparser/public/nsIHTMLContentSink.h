



#ifndef nsIHTMLContentSink_h___
#define nsIHTMLContentSink_h___













































#include "nsIContentSink.h"
#include "nsHTMLTags.h"

#define NS_IHTML_CONTENT_SINK_IID \
  {0xefc5af86, 0x5cfd, 0x4918, {0x9d, 0xd3, 0x5f, 0x7a, 0xb2, 0x88, 0xb2, 0x68}}





class nsIHTMLContentSink : public nsIContentSink 
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTML_CONTENT_SINK_IID)

    



     
  NS_IMETHOD OpenContainer(nsHTMLTag aNodeType) = 0;

  




     
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLContentSink, NS_IHTML_CONTENT_SINK_IID)

#endif 

