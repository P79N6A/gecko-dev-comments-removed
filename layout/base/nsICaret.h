







































#ifndef nsICaret_h__
#define nsICaret_h__

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsCoord.h"
#include "nsFrameSelection.h"

struct nsRect;
struct nsPoint;

class nsIRenderingContext;
class nsIFrame;
class nsIView;
class nsIPresShell;
class nsISelection;
class nsIDOMNode;


#define NS_ICARET_IID \
{ 0x48e23b7f, 0x264e, 0xab7d, \
  { 0x20, 0x03, 0x2c, 0x79, 0x13, 0xe1, 0x09, 0x4d } }


class nsICaret: public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICARET_IID)

  enum EViewCoordinates {
      eTopLevelWindowCoordinates,
      eRenderingViewCoordinates,
      eClosestViewCoordinates,
      eIMECoordinates
    };

  NS_IMETHOD Init(nsIPresShell *inPresShell) = 0;
  NS_IMETHOD Terminate() = 0;
  
  NS_IMETHOD GetCaretDOMSelection(nsISelection **aDOMSel) = 0;
  NS_IMETHOD SetCaretDOMSelection(nsISelection *aDOMSel) = 0;

  


  NS_IMETHOD SetCaretVisible(PRBool inMakeVisible) = 0;

  


  NS_IMETHOD GetCaretVisible(PRBool *outMakeVisible) = 0;

  



  NS_IMETHOD SetCaretReadOnly(PRBool inMakeReadonly) = 0;

  virtual PRBool GetCaretReadOnly() = 0;

  







  NS_IMETHOD GetCaretCoordinates(EViewCoordinates aRelativeToType,
                                 nsISelection *aDOMSel,
                                 nsRect *outCoordinates,
                                 PRBool *outIsCollapsed,
                                 nsIView **outView) = 0;

  


  NS_IMETHOD EraseCaret() = 0;

  


  NS_IMETHOD CheckCaretDrawingState() = 0;

  NS_IMETHOD SetVisibilityDuringSelection(PRBool aVisibilityDuringSelection) = 0;
  
  







  NS_IMETHOD DrawAtPosition(nsIDOMNode* aNode, PRInt32 aOffset) = 0;

  



  NS_IMETHOD GetCaretFrameForNodeOffset(nsIContent* aContentNode,
                                        PRInt32 aOffset,
                                        nsFrameSelection::HINT aFrameHint,
                                        PRUint8 aBidiLevel,
                                        nsIFrame** aReturnFrame,
                                        PRInt32* aReturnOffset) = 0;

  




  virtual nsIFrame *GetCaretFrame() = 0;

  



  virtual nsRect GetCaretRect() = 0;

  


  virtual nsIContent* GetCaretContent() = 0;

  




  virtual void InvalidateOutsideCaret() = 0;

  




  virtual void UpdateCaretPosition() = 0;

  


  virtual void PaintCaret(nsDisplayListBuilder *aBuilder,
                          nsIRenderingContext *aCtx,
                          nsIFrame *aForFrame,
                          const nsPoint &aOffset) = 0;

  







  virtual void SetIgnoreUserModify(PRBool aIgnoreUserModify) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICaret, NS_ICARET_IID)

nsresult
NS_NewCaret(nsICaret** aInstancePtrResult);




class StCaretHider
{
public:
               StCaretHider(nsICaret* aSelCon)
               : mWasVisible(PR_FALSE), mCaret(aSelCon)
               {
                 if (mCaret)
                 {
                   mCaret->GetCaretVisible(&mWasVisible);
                   if (mWasVisible)
                     mCaret->SetCaretVisible(PR_FALSE);
                 }
               }
               
               ~StCaretHider()
               {
                 if (mCaret && mWasVisible)
                   mCaret->SetCaretVisible(PR_TRUE);
                 
               }

protected:

    PRBool                  mWasVisible;
    nsCOMPtr<nsICaret>  mCaret;
};


#endif  

