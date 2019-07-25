




































#ifndef nsITextServicesDocument_h__
#define nsITextServicesDocument_h__

#include "nsISupports.h"

class nsIDOMDocument;
class nsIDOMRange;
class nsIEditor;
class nsString;
class nsITextServicesFilter;





#define NS_ITEXTSERVICESDOCUMENT_IID            \
{ /* 019718E1-CDB5-11d2-8D3C-000000000000 */    \
0x019718e1, 0xcdb5, 0x11d2,                     \
{ 0x8d, 0x3c, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }







class nsITextServicesDocument  : public nsISupports{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEXTSERVICESDOCUMENT_IID)

  typedef enum { eDSNormal=0, eDSUndlerline } TSDDisplayStyle;

  typedef enum { eBlockNotFound=0, 
                 eBlockOutside,    
                 eBlockInside,     
                 eBlockContains,   
                 eBlockPartial     
  } TSDBlockSelectionStatus;

  



  NS_IMETHOD GetDocument(nsIDOMDocument **aDocument) = 0;

  






  NS_IMETHOD InitWithEditor(nsIEditor *aEditor) = 0;

  








  NS_IMETHOD SetExtent(nsIDOMRange* aDOMRange) = 0;

  





  NS_IMETHOD ExpandRangeToWordBoundaries(nsIDOMRange *aRange) = 0;

  



  NS_IMETHOD SetFilter(nsITextServicesFilter *aFilter) = 0;

  




  NS_IMETHOD GetCurrentTextBlock(nsString *aStr) = 0;

  





  NS_IMETHOD FirstBlock() = 0;

  










  NS_IMETHOD LastSelectedBlock(TSDBlockSelectionStatus *aSelectionStatus, PRInt32 *aSelectionOffset, PRInt32 *aSelectionLength) = 0;

  







  NS_IMETHOD PrevBlock() = 0;

  







  NS_IMETHOD NextBlock() = 0;

  









  NS_IMETHOD IsDone(bool *aIsDone) = 0;

  









  NS_IMETHOD SetSelection(PRInt32 aOffset, PRInt32 aLength) = 0;

  



  NS_IMETHOD ScrollSelectionIntoView() = 0;

  





  NS_IMETHOD DeleteSelection() = 0;

  





  NS_IMETHOD InsertText(const nsString *aText) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITextServicesDocument,
                              NS_ITEXTSERVICESDOCUMENT_IID)

#endif 

