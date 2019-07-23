




































#ifndef nsIsIndexFrame_h___
#define nsIsIndexFrame_h___

#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIStatefulFrame.h"
#include "nsIUnicodeEncoder.h"
#include "nsIDOMKeyListener.h"

#include "nsTextControlFrame.h"
typedef   nsTextControlFrame nsNewFrame;

class nsIsIndexFrame : public nsBlockFrame,
                       public nsIAnonymousContentCreator,
                       public nsIDOMKeyListener,
                       public nsIStatefulFrame
{
public:
  nsIsIndexFrame(nsStyleContext* aContext);
  virtual ~nsIsIndexFrame();

  virtual void Destroy(); 

  




  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent) { return NS_OK; }

  




  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent) { return NS_OK; }

  





  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent); 

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  
  virtual PRBool IsLeaf() const;

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  void           SetFocus(PRBool aOn, PRBool aRepaint);

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  NS_IMETHOD OnSubmit(nsPresContext* aPresContext);

  
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState);
  NS_IMETHOD RestoreState(nsPresState* aState);

protected:
  
  
  nsCOMPtr<nsIContent> mTextContent;
  nsCOMPtr<nsIContent> mInputContent;
  nsCOMPtr<nsIContent> mPreHr;
  nsCOMPtr<nsIContent> mPostHr;

private:
  nsresult UpdatePromptLabel(PRBool aNotify);
  nsresult GetInputFrame(nsIFormControlFrame** oFrame);
  void GetInputValue(nsString& oString);
  void SetInputValue(const nsString& aString);

  void GetSubmitCharset(nsCString& oCharset);
  NS_IMETHOD GetEncoder(nsIUnicodeEncoder** encoder);
  char* UnicodeToNewBytes(const PRUnichar* aSrc, PRUint32 aLen, nsIUnicodeEncoder* encoder);
  void URLEncode(const nsString& aString, nsIUnicodeEncoder* encoder, nsString& oString);

  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
};

#endif


