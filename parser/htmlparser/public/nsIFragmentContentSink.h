



































#ifndef nsIFragmentContentSink_h___
#define nsIFragmentContentSink_h___

#include "nsISupports.h"

class nsIDOMDocumentFragment;
class nsIDocument;

#define NS_I_FRAGMENT_CONTENT_SINK_IID \
  { 0x7c78cbad, 0xdaf5, 0x487e, \
    { 0x96, 0x98, 0xab, 0xcc, 0x21, 0x5a, 0x8d, 0x39 } }







class nsIFragmentContentSink : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_I_FRAGMENT_CONTENT_SINK_IID)
  





  NS_IMETHOD FinishFragmentParsing(nsIDOMDocumentFragment** aFragment) = 0;

  







  NS_IMETHOD SetTargetDocument(nsIDocument* aDocument) = 0;

  



  NS_IMETHOD WillBuildContent() = 0;

  




  NS_IMETHOD DidBuildContent() = 0;

  





  NS_IMETHOD IgnoreFirstContainer() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFragmentContentSink,
                              NS_I_FRAGMENT_CONTENT_SINK_IID)





class nsIParanoidFragmentContentSink : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_I_PARANOID_FRAGMENT_CONTENT_SINK_IID)

  


  virtual void AllowStyles() = 0;

  


  virtual void AllowComments() = 0;
};

nsresult
NS_NewXMLFragmentContentSink(nsIFragmentContentSink** aInstancePtrResult);

#endif
