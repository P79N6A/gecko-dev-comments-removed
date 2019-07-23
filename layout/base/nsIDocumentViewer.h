






































#ifndef nsIDocumentViewer_h___
#define nsIDocumentViewer_h___

#include "nsIContentViewer.h"

class nsIDocument;
class nsPresContext;
class nsIPresShell;
class nsIStyleSheet;

#define NS_IDOCUMENT_VIEWER_IID \
  { 0xf29e5537, 0x0763, 0x4977, \
    { 0x83, 0xc2, 0x3c, 0x93, 0x6c, 0x66, 0xa9, 0xfc } }




class nsIDocumentViewer : public nsIContentViewer
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_VIEWER_IID)
  
  NS_IMETHOD GetPresShell(nsIPresShell** aResult) = 0;
  
  NS_IMETHOD GetPresContext(nsPresContext** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentViewer, NS_IDOCUMENT_VIEWER_IID)

#endif 
