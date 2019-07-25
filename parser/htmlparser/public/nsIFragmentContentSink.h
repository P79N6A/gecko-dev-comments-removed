



































#ifndef nsIFragmentContentSink_h___
#define nsIFragmentContentSink_h___

#include "nsISupports.h"

class nsIDOMDocumentFragment;
class nsIDocument;

#define NS_I_FRAGMENT_CONTENT_SINK_IID \
  { 0x1a8ce30b, 0x63fc, 0x441a, \
    { 0xa3, 0xaa, 0xf7, 0x16, 0xc0, 0xfe, 0x96, 0x69 } }







class nsIFragmentContentSink : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_I_FRAGMENT_CONTENT_SINK_IID)
  





  NS_IMETHOD FinishFragmentParsing(nsIDOMDocumentFragment** aFragment) = 0;

  







  NS_IMETHOD SetTargetDocument(nsIDocument* aDocument) = 0;

  



  NS_IMETHOD WillBuildContent() = 0;

  




  NS_IMETHOD DidBuildContent() = 0;

  





  NS_IMETHOD IgnoreFirstContainer() = 0;

  


  NS_IMETHOD SetPreventScriptExecution(bool aPreventScriptExecution) = 0;
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
