




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


#ifdef KeyPress
#undef KeyPress
#endif

class nsFormHistory;
class nsINode;

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
  bool RowMatch(nsFormHistory *aHistory, uint32_t aIndex, const nsAString &aInputName, const nsAString &aInputValue);

  inline nsIDocShell *GetDocShellForInput(nsIDOMHTMLInputElement *aInput);
  inline nsIDOMWindow *GetWindowForDocShell(nsIDocShell *aDocShell);
  inline int32_t GetIndexOfDocShell(nsIDocShell *aDocShell);

  void MaybeRemoveMutationObserver(nsINode* aNode);

  static PLDHashOperator RemoveForDocumentEnumerator(const nsINode* aKey,
                                                     bool& aEntry,
                                                     void* aUserData);
  bool IsEventTrusted(nsIDOMEvent *aEvent);
  

  nsCOMPtr<nsIAutoCompleteController> mController;
  nsCOMPtr<nsILoginManager> mLoginManager;
  nsIDOMHTMLInputElement* mFocusedInput;
  nsINode* mFocusedInputNode;
  nsINode* mListNode;
  nsCOMPtr<nsIAutoCompletePopup> mFocusedPopup;

  nsCOMPtr<nsISupportsArray> mDocShells;
  nsCOMPtr<nsISupportsArray> mPopups;

  
  nsCOMPtr<nsIAutoCompleteResult> mLastSearchResult;
  nsCOMPtr<nsIAutoCompleteObserver> mLastListener;
  nsString mLastSearchString;

  nsDataHashtable<nsPtrHashKey<const nsINode>, bool> mPwmgrInputs;

  uint32_t mTimeout;
  uint32_t mMinResultsForPopup;
  uint32_t mMaxRows;
  bool mDisableAutoComplete;
  bool mCompleteDefaultIndex;
  bool mCompleteSelectedIndex;
  bool mForceComplete;
  bool mSuppressOnInput;
};

#endif 
