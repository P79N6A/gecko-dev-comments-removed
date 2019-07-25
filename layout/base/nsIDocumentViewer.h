






































#ifndef nsIDocumentViewer_h___
#define nsIDocumentViewer_h___

#include "nsIContentViewer.h"

class nsIDocument;
class nsPresContext;
class nsIPresShell;
class nsIStyleSheet;
class nsIView;

class nsDOMNavigationTiming;

#define NS_IDOCUMENT_VIEWER_IID \
  { 0x5a5c9a1d, 0x49c4, 0x4f3f, \
    { 0x80, 0xcd, 0x12, 0x09, 0x5b, 0x1e, 0x1f, 0x61 } }





class nsIDocumentViewer : public nsIContentViewer
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_VIEWER_IID)
  
  NS_IMETHOD GetPresShell(nsIPresShell** aResult) = 0;
  
  NS_IMETHOD GetPresContext(nsPresContext** aResult) = 0;

  NS_IMETHOD SetDocumentInternal(nsIDocument* aDocument,
                                 PRBool aForceReuseInnerWindow) = 0;

  virtual nsIView* FindContainerView() = 0;

  virtual void SetNavigationTiming(nsDOMNavigationTiming* timing) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentViewer, NS_IDOCUMENT_VIEWER_IID)

#endif 
