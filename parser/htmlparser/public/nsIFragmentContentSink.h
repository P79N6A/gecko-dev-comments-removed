



































#ifndef nsIFragmentContentSink_h___
#define nsIFragmentContentSink_h___

#include "nsISupports.h"

class nsIDOMDocumentFragment;
class nsIDocument;

#define NS_I_FRAGMENT_CONTENT_SINK_IID \
  { 0x2cec7263, 0x9dd0, 0x4413, \
    { 0xb6, 0x68, 0x6f, 0xf0, 0xa1, 0x40, 0xc1, 0xbe } }







class nsIFragmentContentSink : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_I_FRAGMENT_CONTENT_SINK_IID)
  





  NS_IMETHOD GetFragment(nsIDOMDocumentFragment** aFragment) = 0;

  







  NS_IMETHOD SetTargetDocument(nsIDocument* aDocument) = 0;

  



  NS_IMETHOD WillBuildContent() = 0;

  




  NS_IMETHOD DidBuildContent() = 0;

  





  NS_IMETHOD IgnoreFirstContainer() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFragmentContentSink,
                              NS_I_FRAGMENT_CONTENT_SINK_IID)







#define NS_HTMLFRAGMENTSINK_CONTRACTID "@mozilla.org/layout/htmlfragmentsink;1"
#define NS_HTMLFRAGMENTSINK2_CONTRACTID "@mozilla.org/layout/htmlfragmentsink;2"
#define NS_HTMLPARANOIDFRAGMENTSINK_CONTRACTID \
"@mozilla.org/htmlparanoidfragmentsink;1"

#define NS_XMLFRAGMENTSINK_CONTRACTID "@mozilla.org/layout/xmlfragmentsink;1"
#define NS_XMLFRAGMENTSINK2_CONTRACTID "@mozilla.org/layout/xmlfragmentsink;2"
#define NS_XHTMLPARANOIDFRAGMENTSINK_CONTRACTID \
"@mozilla.org/xhtmlparanoidfragmentsink;1"



nsresult
NS_NewXMLFragmentContentSink(nsIFragmentContentSink** aInstancePtrResult);
nsresult
NS_NewXMLFragmentContentSink2(nsIFragmentContentSink** aInstancePtrResult);



nsresult
NS_NewXHTMLParanoidFragmentSink(nsIFragmentContentSink** aInstancePtrResult);
void
NS_XHTMLParanoidFragmentSinkShutdown();
#endif
