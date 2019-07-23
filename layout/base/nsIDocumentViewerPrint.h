



































#ifndef nsIDocumentViewerPrint_h___
#define nsIDocumentViewerPrint_h___

#include "nsISupports.h"

class nsIDocument;
class nsStyleSet;


#define NS_IDOCUMENT_VIEWER_PRINT_IID \
 { 0xd0b7f354, 0xd575, 0x43fd, { 0x90, 0x3d, 0x5a, 0xa3, 0x5a, 0x19, 0x3e, 0xda } }





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
  virtual void     OnDonePrinting();

#endif 
