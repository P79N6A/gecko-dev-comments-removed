



































#ifndef nsIHTMLFragmentContentSink_h___
#define nsIHTMLFragmentContentSink_h___

#include "nsIHTMLContentSink.h"

#define NS_HTMLFRAGMENTSINK_CONTRACTID "@mozilla.org/layout/htmlfragmentsink;1"
#define NS_HTMLFRAGMENTSINK2_CONTRACTID "@mozilla.org/layout/htmlfragmentsink;2"

class nsIDOMDocumentFragment;
class nsIDocument;

#define NS_IHTML_FRAGMENT_CONTENT_SINK_IID \
 {0xa6cf9102, 0x15b3, 0x11d2,              \
 {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsIHTMLFragmentContentSink : public nsIHTMLContentSink {
public:
  





  NS_IMETHOD GetFragment(nsIDOMDocumentFragment** aFragment) = 0;

  









  NS_IMETHOD SetTargetDocument(nsIDocument* aDocument) = 0;
};

#endif
