






































#ifndef __nsFormFillController__
#define __nsFormFillController__

#include "nsIFormFillController.h"
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteController.h"
#include "nsIAutoCompletePopup.h"
#include "nsIDOMEventListener.h"
#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsDataHashtable.h"
#include "nsIDocShell.h"
#include "nsIDOMWindow.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsILoginManager.h"
#include "nsIMutationObserver.h"

class nsFormHistory;

class nsFormFillController : public nsIFormFillController,
                             public nsIAutoCompleteInput,
                             public nsIAutoCompleteSearch,
                             public nsIDOMEventListener,
                             public nsIMutationObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFORMFILLCONTROLLER
  NS_DECL_NSIAUTOCOMPLETESEARCH
  NS_DECL_NSIAUTOCOMPLETEINPUT
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIMUTATIONOBSERVER

  nsresult Focus(nsIDOMEvent* aEvent);
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);
  nsresult MouseDown(nsIDOMEvent* aMouseEvent);

  nsFormFillController();
  virtual ~nsFormFillController();

protected:
  void AddWindowListeners(nsIDOMWindow *aWindow);
  void RemoveWindowListeners(nsIDOMWindow *aWindow);
  
  void AddKeyListener(nsIDOMHTMLInputElement *aInput);
  void RemoveKeyListener();
  
  void StartControllingInput(nsIDOMHTMLInputElement *aInput);
  void StopControllingInput();
  
  void RevalidateDataList();
  PRBool RowMatch(nsFormHistory *aHistory, PRUint32 aIndex, const nsAString &aInputName, const nsAString &aInputValue);
  
  inline nsIDocShell *GetDocShellForInput(nsIDOMHTMLInputElement *aInput);
  inline nsIDOMWindow *GetWindowForDocShell(nsIDocShell *aDocShell);
  inline PRInt32 GetIndexOfDocShell(nsIDocShell *aDocShell);

  static PLDHashOperator RemoveForDOMDocumentEnumerator(nsISupports* aKey,
                                                        PRInt32& aEntry,
                                                        void* aUserData);
  PRBool IsEventTrusted(nsIDOMEvent *aEvent);
  PRBool IsInputAutoCompleteOff();
  

  nsCOMPtr<nsIAutoCompleteController> mController;
  nsCOMPtr<nsILoginManager> mLoginManager;
  nsCOMPtr<nsIDOMHTMLInputElement> mFocusedInput;
  nsCOMPtr<nsIAutoCompletePopup> mFocusedPopup;

  nsCOMPtr<nsISupportsArray> mDocShells;
  nsCOMPtr<nsISupportsArray> mPopups;

  
  nsCOMPtr<nsIAutoCompleteResult> mLastSearchResult;
  nsCOMPtr<nsIAutoCompleteObserver> mLastListener;
  nsString mLastSearchString;

  nsDataHashtable<nsISupportsHashKey,PRInt32> mPwmgrInputs;

  PRUint32 mTimeout;
  PRUint32 mMinResultsForPopup;
  PRUint32 mMaxRows;
  PRPackedBool mDisableAutoComplete; 
  PRPackedBool mCompleteDefaultIndex;
  PRPackedBool mCompleteSelectedIndex;
  PRPackedBool mForceComplete;
  PRPackedBool mSuppressOnInput;
};

#endif 
