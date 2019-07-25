



































#ifndef nsIDocumentViewerPrint_h___
#define nsIDocumentViewerPrint_h___

#include "nsISupports.h"

class nsIDocument;
class nsStyleSet;
class nsIPresShell;
class nsPresContext;
class nsIWidget;
class nsIViewManager;


#define NS_IDOCUMENT_VIEWER_PRINT_IID \
{ 0xc6f255cf, 0xcadd, 0x4382, \
  { 0xb5, 0x7f, 0xcd, 0x2a, 0x98, 0x74, 0x16, 0x9b } }





class nsIDocumentViewerPrint : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_VIEWER_PRINT_IID)

  virtual void SetIsPrinting(PRBool aIsPrinting) = 0;
  virtual PRBool GetIsPrinting() = 0;

  virtual void SetIsPrintPreview(PRBool aIsPrintPreview) = 0;
  virtual PRBool GetIsPrintPreview() = 0;

  
  
  
  virtual nsresult CreateStyleSet(nsIDocument* aDocument, nsStyleSet** aStyleSet) = 0;

  virtual void IncrementDestroyRefCount() = 0;

  virtual void ReturnToGalleyPresentation() = 0;

  virtual void OnDonePrinting() = 0;

  


  virtual PRBool IsInitializedForPrintPreview() = 0;

  


  virtual void InitializeForPrintPreview() = 0;

  


  virtual void SetPrintPreviewPresentation(nsIWidget* aWidget,
                                           nsIViewManager* aViewManager,
                                           nsPresContext* aPresContext,
                                           nsIPresShell* aPresShell) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentViewerPrint,
                              NS_IDOCUMENT_VIEWER_PRINT_IID)


#define NS_DECL_NSIDOCUMENTVIEWERPRINT \
  virtual void     SetIsPrinting(PRBool aIsPrinting); \
  virtual PRBool   GetIsPrinting(); \
  virtual void     SetIsPrintPreview(PRBool aIsPrintPreview); \
  virtual PRBool   GetIsPrintPreview(); \
  virtual nsresult CreateStyleSet(nsIDocument* aDocument, nsStyleSet** aStyleSet); \
  virtual void     IncrementDestroyRefCount(); \
  virtual void     ReturnToGalleyPresentation(); \
  virtual void     OnDonePrinting(); \
  virtual PRBool   IsInitializedForPrintPreview(); \
  virtual void     InitializeForPrintPreview(); \
  virtual void     SetPrintPreviewPresentation(nsIWidget* aWidget, \
                                               nsIViewManager* aViewManager, \
                                               nsPresContext* aPresContext, \
                                               nsIPresShell* aPresShell);

#endif 
