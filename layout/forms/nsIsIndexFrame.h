




































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
                       public nsIStatefulFrame
{
public:
  nsIsIndexFrame(nsStyleContext* aContext);
  virtual ~nsIsIndexFrame();

  virtual void Destroy(); 

private:
  void KeyPress(nsIDOMEvent* aKeyEvent);

  class KeyListener : public nsIDOMKeyListener
  {
    NS_DECL_ISUPPORTS

    KeyListener(nsIsIndexFrame* aOwner) : mOwner(aOwner) { };

    NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent) { return NS_OK; }

    NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent) { return NS_OK; }

    NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent); 

    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

    nsIsIndexFrame* mOwner;
  };

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
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

  nsCOMPtr<KeyListener> mListener;
};

#endif


