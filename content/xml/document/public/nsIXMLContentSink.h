



































#ifndef nsIXMLContentSink_h___
#define nsIXMLContentSink_h___

#include "nsIContentSink.h"
#include "nsIParserNode.h"
#include "nsISupports.h"

class nsIDocument;
class nsIURI;
class nsIChannel;

#define NS_IXMLCONTENT_SINK_IID \
 { 0x63fedea0, 0x9b0f, 0x4d64, \
 { 0x9b, 0xa5, 0x37, 0xc6, 0x99, 0x73, 0x29, 0x35 } }
























class nsIXMLContentSink : public nsIContentSink {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXMLCONTENT_SINK_IID)

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXMLContentSink, NS_IXMLCONTENT_SINK_IID)

nsresult
NS_NewXMLContentSink(nsIXMLContentSink** aInstancePtrResult, nsIDocument* aDoc,
                     nsIURI* aURL, nsISupports* aContainer,
                     nsIChannel *aChannel);

#endif
