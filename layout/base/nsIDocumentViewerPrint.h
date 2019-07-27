



#ifndef nsIDocumentViewerPrint_h___
#define nsIDocumentViewerPrint_h___

#include "nsISupports.h"

class nsIDocument;
class nsStyleSet;
class nsIPresShell;
class nsPresContext;
class nsViewManager;


#define NS_IDOCUMENT_VIEWER_PRINT_IID \
{ 0xc6f255cf, 0xcadd, 0x4382, \
  { 0xb5, 0x7f, 0xcd, 0x2a, 0x98, 0x74, 0x16, 0x9b } }





class nsIDocumentViewerPrint : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_VIEWER_PRINT_IID)

  virtual void SetIsPrinting(bool aIsPrinting) = 0;
  virtual bool GetIsPrinting() = 0;

  virtual void SetIsPrintPreview(bool aIsPrintPreview) = 0;
  virtual bool GetIsPrintPreview() = 0;

  
  
  
  virtual nsresult CreateStyleSet(nsIDocument* aDocument, nsStyleSet** aStyleSet) = 0;

  virtual void IncrementDestroyRefCount() = 0;

  virtual void ReturnToGalleyPresentation() = 0;

  virtual void OnDonePrinting() = 0;

  


  virtual bool IsInitializedForPrintPreview() = 0;

  


  virtual void InitializeForPrintPreview() = 0;

  


  virtual void SetPrintPreviewPresentation(nsViewManager* aViewManager,
                                           nsPresContext* aPresContext,
                                           nsIPresShell* aPresShell) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentViewerPrint,
                              NS_IDOCUMENT_VIEWER_PRINT_IID)


#define NS_DECL_NSIDOCUMENTVIEWERPRINT \
  virtual void     SetIsPrinting(bool aIsPrinting) override; \
  virtual bool     GetIsPrinting() override; \
  virtual void     SetIsPrintPreview(bool aIsPrintPreview) override; \
  virtual bool     GetIsPrintPreview() override; \
  virtual nsresult CreateStyleSet(nsIDocument* aDocument, nsStyleSet** aStyleSet) override; \
  virtual void     IncrementDestroyRefCount() override; \
  virtual void     ReturnToGalleyPresentation() override; \
  virtual void     OnDonePrinting() override; \
  virtual bool     IsInitializedForPrintPreview() override; \
  virtual void     InitializeForPrintPreview() override; \
  virtual void     SetPrintPreviewPresentation(nsViewManager* aViewManager, \
                                               nsPresContext* aPresContext, \
                                               nsIPresShell* aPresShell) override;

#endif 
