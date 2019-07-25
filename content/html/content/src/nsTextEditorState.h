





































#ifndef nsTextEditorState_h__
#define nsTextEditorState_h__

#include "nsAutoPtr.h"
#include "nsITextControlElement.h"
#include "nsCycleCollectionParticipant.h"

class nsTextInputListener;
class nsTextControlFrame;
class nsTextInputSelectionImpl;
class nsAnonDivObserver;
class nsISelectionController;
class nsFrameSelection;
class nsIEditor;
class nsITextControlElement;
struct SelectionState;
























































































class nsTextEditorState {
public:
  explicit nsTextEditorState(nsITextControlElement* aOwningElement);
  ~nsTextEditorState();

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsTextEditorState)
  NS_INLINE_DECL_REFCOUNTING(nsTextEditorState)

  nsIEditor* GetEditor();
  nsISelectionController* GetSelectionController() const;
  nsFrameSelection* GetConstFrameSelection();
  nsresult BindToFrame(nsTextControlFrame* aFrame);
  void UnbindFromFrame(nsTextControlFrame* aFrame);
  nsresult PrepareEditor(const nsAString *aValue = nsnull);
  void InitializeKeyboardEventListeners();

  void SetValue(const nsAString& aValue, PRBool aUserInput);
  void GetValue(nsAString& aValue, PRBool aIgnoreWrap) const;
  void EmptyValue() { if (mValue) mValue->Truncate(); }
  PRBool IsEmpty() const { return mValue ? mValue->IsEmpty() : PR_TRUE; }

  nsresult CreatePlaceholderNode();

  nsIContent* GetRootNode() {
    if (!mRootNode)
      CreateRootNode();
    return mRootNode;
  }
  nsIContent* GetPlaceholderNode() {
    return mPlaceholderDiv;
  }

  PRBool IsSingleLineTextControl() const {
    return mTextCtrlElement->IsSingleLineTextControl();
  }
  PRBool IsTextArea() const {
    return mTextCtrlElement->IsTextArea();
  }
  PRBool IsPlainTextControl() const {
    return mTextCtrlElement->IsPlainTextControl();
  }
  PRBool IsPasswordTextControl() const {
    return mTextCtrlElement->IsPasswordTextControl();
  }
  PRInt32 GetCols() {
    return mTextCtrlElement->GetCols();
  }
  PRInt32 GetWrapCols() {
    return mTextCtrlElement->GetWrapCols();
  }
  PRInt32 GetRows() {
    return mTextCtrlElement->GetRows();
  }

  
  void SetPlaceholderClass(PRBool aVisible, PRBool aNotify);
  void UpdatePlaceholderText(PRBool aNotify); 

  




  PRBool GetMaxLength(PRInt32* aMaxLength);

  
  static NS_HIDDEN_(void) ShutDown();

  void ClearValueCache() { mCachedValue.Truncate(); }

private:
  
  nsTextEditorState(const nsTextEditorState&);
  
  void operator= (const nsTextEditorState&);

  nsresult CreateRootNode();

  void ValueWasChanged(PRBool aNotify);

  void DestroyEditor();
  void Clear();

  class InitializationGuard {
  public:
    explicit InitializationGuard(nsTextEditorState& aState) :
      mState(aState),
      mGuardSet(PR_FALSE)
    {
      if (!mState.mInitializing) {
        mGuardSet = PR_TRUE;
        mState.mInitializing = PR_TRUE;
      }
    }
    ~InitializationGuard() {
      if (mGuardSet) {
        mState.mInitializing = PR_FALSE;
      }
    }
    PRBool IsInitializingRecursively() const {
      return !mGuardSet;
    }
  private:
    nsTextEditorState& mState;
    PRBool mGuardSet;
  };
  friend class InitializationGuard;

  nsITextControlElement* const mTextCtrlElement;
  nsRefPtr<nsTextInputSelectionImpl> mSelCon;
  nsAutoPtr<SelectionState> mSelState;
  nsCOMPtr<nsIEditor> mEditor;
  nsCOMPtr<nsIContent> mRootNode;
  nsCOMPtr<nsIContent> mPlaceholderDiv;
  nsTextControlFrame* mBoundFrame;
  nsTextInputListener* mTextListener;
  nsAutoPtr<nsCString> mValue;
  nsRefPtr<nsAnonDivObserver> mMutationObserver;
  mutable nsString mCachedValue; 
  PRPackedBool mEditorInitialized;
  PRPackedBool mInitializing; 
};

#endif
