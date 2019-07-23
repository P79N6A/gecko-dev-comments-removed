






































#ifndef __nsFormFillController__
#define __nsFormFillController__

#include "nsIFormFillController.h"
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteController.h"
#include "nsIAutoCompletePopup.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMCompositionListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsIDocShell.h"
#include "nsIDOMWindow.h"
#include "nsIDOMHTMLInputElement.h"

class nsFormHistory;

class nsFormFillController : public nsIFormFillController,
                             public nsIAutoCompleteInput,
                             public nsIAutoCompleteSearch,
                             public nsIDOMFocusListener,
                             public nsIDOMKeyListener,
                             public nsIDOMCompositionListener,
                             public nsIDOMFormListener,
                             public nsIDOMMouseListener,
                             public nsIDOMLoadListener,
                             public nsIDOMContextMenuListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFORMFILLCONTROLLER
  NS_DECL_NSIAUTOCOMPLETESEARCH
  NS_DECL_NSIAUTOCOMPLETEINPUT
  NS_DECL_NSIDOMEVENTLISTENER

  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);

  
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);

  
  NS_IMETHOD HandleStartComposition(nsIDOMEvent* aCompositionEvent);
  NS_IMETHOD HandleEndComposition(nsIDOMEvent* aCompositionEvent);
  NS_IMETHOD HandleQueryComposition(nsIDOMEvent* aCompositionEvent);
  NS_IMETHOD HandleQueryReconversion(nsIDOMEvent* aCompositionEvent);
  NS_IMETHOD HandleQueryCaretRect(nsIDOMEvent* aCompositionEvent);

  
  NS_IMETHOD Submit(nsIDOMEvent* aEvent);
  NS_IMETHOD Reset(nsIDOMEvent* aEvent);
  NS_IMETHOD Change(nsIDOMEvent* aEvent);
  NS_IMETHOD Select(nsIDOMEvent* aEvent);
  NS_IMETHOD Input(nsIDOMEvent* aEvent);

  
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

  
  NS_IMETHOD Load(nsIDOMEvent *aLoadEvent);
  NS_IMETHOD BeforeUnload(nsIDOMEvent *aLoadEvent);
  NS_IMETHOD Unload(nsIDOMEvent *aLoadEvent);
  NS_IMETHOD Abort(nsIDOMEvent *aLoadEvent);
  NS_IMETHOD Error(nsIDOMEvent *aLoadEvent);

  
  NS_IMETHOD ContextMenu(nsIDOMEvent* aContextMenuEvent);

  nsFormFillController();
  virtual ~nsFormFillController();

protected:
  void AddWindowListeners(nsIDOMWindow *aWindow);
  void RemoveWindowListeners(nsIDOMWindow *aWindow);
  
  void AddKeyListener(nsIDOMHTMLInputElement *aInput);
  void RemoveKeyListener();
  
  void StartControllingInput(nsIDOMHTMLInputElement *aInput);
  void StopControllingInput();
  
  PRBool RowMatch(nsFormHistory *aHistory, PRUint32 aIndex, const nsAString &aInputName, const nsAString &aInputValue);
  
  inline nsIDocShell *GetDocShellForInput(nsIDOMHTMLInputElement *aInput);
  inline nsIDOMWindow *GetWindowForDocShell(nsIDocShell *aDocShell);
  inline PRInt32 GetIndexOfDocShell(nsIDocShell *aDocShell);

  

  nsCOMPtr<nsIAutoCompleteController> mController;
  nsCOMPtr<nsIDOMHTMLInputElement> mFocusedInput;
  nsCOMPtr<nsIAutoCompletePopup> mFocusedPopup;

  nsCOMPtr<nsISupportsArray> mDocShells;
  nsCOMPtr<nsISupportsArray> mPopups;
  
  PRUint32 mTimeout;
  PRUint32 mMinResultsForPopup;
  PRUint32 mMaxRows;
  PRPackedBool mDisableAutoComplete; 
  PRPackedBool mCompleteDefaultIndex;
  PRPackedBool mCompleteSelectedIndex;
  PRPackedBool mForceComplete;
  PRPackedBool mSuppressOnInput;
  PRPackedBool mIgnoreClick;
};

#endif 
