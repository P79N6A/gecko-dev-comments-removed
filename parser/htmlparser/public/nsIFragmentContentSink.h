



































#ifndef nsIFragmentContentSink_h___
#define nsIFragmentContentSink_h___

#include "nsISupports.h"

class nsIDOMDocumentFragment;
class nsIDocument;

#define NS_I_FRAGMENT_CONTENT_SINK_IID \
  { 0x1ecdb30d, 0x1f10, 0x45d2, \
    { 0xa4, 0xf4, 0xec, 0xbc, 0x03, 0x52, 0x9a, 0x7e } }







class nsIFragmentContentSink : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_I_FRAGMENT_CONTENT_SINK_IID)
  







  NS_IMETHOD GetFragment(PRBool aWillOwnFragment,
                         nsIDOMDocumentFragment** aFragment) = 0;

  







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
