






































#ifndef nsIDocumentViewer_h___
#define nsIDocumentViewer_h___

#include "nsIContentViewer.h"

class nsIDocument;
class nsPresContext;
class nsIPresShell;
class nsIStyleSheet;

#define NS_IDOCUMENT_VIEWER_IID \
 { 0xf81fc126, 0x6693, 0x4bc5,{0xa7, 0xe9, 0xfc, 0xb0, 0x76, 0xd9, 0x06, 0x6d} }





class nsIDocumentViewer : public nsIContentViewer
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_VIEWER_IID)

  NS_IMETHOD GetDocument(nsIDocument** aResult) = 0;
  
  NS_IMETHOD GetPresShell(nsIPresShell** aResult) = 0;
  
  NS_IMETHOD GetPresContext(nsPresContext** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentViewer, NS_IDOCUMENT_VIEWER_IID)

#endif 
