






































#ifndef nsIDocumentViewer_h___
#define nsIDocumentViewer_h___

#include "nsIContentViewer.h"

class nsIDocument;
class nsPresContext;
class nsIPresShell;
class nsIStyleSheet;

#define NS_IDOCUMENT_VIEWER_IID \
 { 0x41796e63, 0xbd1f, 0x401d,{0xb6, 0x63, 0x5b, 0x86, 0xa9, 0x70, 0x72, 0x31}}





class nsIDocumentViewer : public nsIContentViewer
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_VIEWER_IID)

  NS_IMETHOD SetUAStyleSheet(nsIStyleSheet* aUAStyleSheet) = 0;
  
  NS_IMETHOD GetDocument(nsIDocument** aResult) = 0;
  
  NS_IMETHOD GetPresShell(nsIPresShell** aResult) = 0;
  
  NS_IMETHOD GetPresContext(nsPresContext** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentViewer, NS_IDOCUMENT_VIEWER_IID)

#endif 
