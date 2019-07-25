





































#ifndef nsTextEditorState_h__
#define nsTextEditorState_h__

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsITextControlElement.h"
#include "nsITextControlFrame.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIContent.h"

class nsTextInputListener;
class nsTextControlFrame;
class nsTextInputSelectionImpl;
class nsAnonDivObserver;
class nsISelectionController;
class nsFrameSelection;
class nsIEditor;
class nsITextControlElement;





























































































class RestoreSelectionState;

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

  void SetValue(const nsAString& aValue, bool aUserInput);
  void GetValue(nsAString& aValue, bool aIgnoreWrap) const;
  void EmptyValue() { if (mValue) mValue->Truncate(); }
  bool IsEmpty() const { return mValue ? mValue->IsEmpty() : true; }

  nsresult CreatePlaceholderNode();

  nsIContent* GetRootNode() {
    if (!mRootNode)
      CreateRootNode();
    return mRootNode;
  }
  nsIContent* GetPlaceholderNode() {
    return mPlaceholderDiv;
  }

  bool IsSingleLineTextControl() const {
    return mTextCtrlElement->IsSingleLineTextControl();
  }
  bool IsTextArea() const {
    return mTextCtrlElement->IsTextArea();
  }
  bool IsPlainTextControl() const {
    return mTextCtrlElement->IsPlainTextControl();
  }
  bool IsPasswordTextControl() const {
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

  
  void SetPlaceholderClass(bool aVisible, bool aNotify);
  void UpdatePlaceholderText(bool aNotify); 

  




  bool GetMaxLength(PRInt32* aMaxLength);

  
  static NS_HIDDEN_(void) ShutDown();

  void ClearValueCache() { mCachedValue.Truncate(); }

  void HideSelectionIfBlurred();

  struct SelectionProperties {
    SelectionProperties() : mStart(0), mEnd(0),
      mDirection(nsITextControlFrame::eForward) {}
    bool IsDefault() const {
      return mStart == 0 && mEnd == 0 &&
             mDirection == nsITextControlFrame::eForward;
    }
    PRInt32 mStart, mEnd;
    nsITextControlFrame::SelectionDirection mDirection;
  };

  bool IsSelectionCached() const { return mSelectionCached; }
  SelectionProperties& GetSelectionProperties() {
    return mSelectionProperties;
  }
  void WillInitEagerly() { mSelectionRestoreEagerInit = true; }
  bool HasNeverInitializedBefore() const { return !mEverInited; }

  void UpdateEditableState(bool aNotify) {
    if (mRootNode) {
      mRootNode->UpdateEditableState(aNotify);
    }
  }

private:
  friend class RestoreSelectionState;

  
  nsTextEditorState(const nsTextEditorState&);
  
  void operator= (const nsTextEditorState&);

  nsresult CreateRootNode();

  void ValueWasChanged(bool aNotify);

  void DestroyEditor();
  void Clear();

  nsresult InitializeRootNode();

  void FinishedRestoringSelection() { mRestoringSelection = nsnull; }

  class InitializationGuard {
  public:
    explicit InitializationGuard(nsTextEditorState& aState) :
      mState(aState),
      mGuardSet(false)
    {
      if (!mState.mInitializing) {
        mGuardSet = true;
        mState.mInitializing = true;
      }
    }
    ~InitializationGuard() {
      if (mGuardSet) {
        mState.mInitializing = false;
      }
    }
    bool IsInitializingRecursively() const {
      return !mGuardSet;
    }
  private:
    nsTextEditorState& mState;
    bool mGuardSet;
  };
  friend class InitializationGuard;
  friend class PrepareEditorEvent;

  nsITextControlElement* const mTextCtrlElement;
  nsRefPtr<nsTextInputSelectionImpl> mSelCon;
  RestoreSelectionState* mRestoringSelection;
  nsCOMPtr<nsIEditor> mEditor;
  nsCOMPtr<nsIContent> mRootNode;
  nsCOMPtr<nsIContent> mPlaceholderDiv;
  nsTextControlFrame* mBoundFrame;
  nsTextInputListener* mTextListener;
  nsAutoPtr<nsCString> mValue;
  nsRefPtr<nsAnonDivObserver> mMutationObserver;
  mutable nsString mCachedValue; 
  bool mEverInited; 
  bool mEditorInitialized;
  bool mInitializing; 
  bool mValueTransferInProgress; 
  bool mSelectionCached; 
  mutable bool mSelectionRestoreEagerInit; 
  SelectionProperties mSelectionProperties;
};

#endif
