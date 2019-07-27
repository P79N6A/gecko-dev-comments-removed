





#ifndef mozilla_TextComposition_h
#define mozilla_TextComposition_h

#include "nsCOMPtr.h"
#include "nsINode.h"
#include "nsIWeakReference.h"
#include "nsIWidget.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsPresContext.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TextRange.h"

class nsIEditor;

namespace mozilla {

class EventDispatchingCallback;
class IMEStateManager;







class TextComposition final
{
  friend class IMEStateManager;

  NS_INLINE_DECL_REFCOUNTING(TextComposition)

public:
  TextComposition(nsPresContext* aPresContext,
                  nsINode* aNode,
                  WidgetCompositionEvent* aCompositionEvent);

  bool Destroyed() const { return !mPresContext; }
  nsPresContext* GetPresContext() const { return mPresContext; }
  nsINode* GetEventTargetNode() const { return mNode; }
  
  
  const nsString& LastData() const { return mLastData; }
  
  
  
  
  
  
  const nsString& String() const { return mString; }
  
  
  
  
  
  TextRangeArray* GetRanges() const { return mRanges; }
  
  nsIWidget* GetWidget() const
  {
    return mPresContext ? mPresContext->GetRootWidget() : nullptr;
  }
  
  
  bool IsSynthesizedForTests() const { return mIsSynthesizedForTests; }

  bool MatchesNativeContext(nsIWidget* aWidget) const;

  


  void Destroy();

  



  nsresult RequestToCommit(nsIWidget* aWidget, bool aDiscard);

  



  nsresult NotifyIME(widget::IMEMessage aMessage);

  


  uint32_t NativeOffsetOfStartComposition() const
  {
    return mCompositionStartOffset;
  }

  


  uint32_t OffsetOfTargetClause() const { return mCompositionTargetOffset; }

  



  bool IsComposing() const { return mIsComposing; }

  



  bool IsEditorHandlingEvent() const
  {
    return mIsEditorHandlingEvent;
  }

  



  void StartHandlingComposition(nsIEditor* aEditor);
  void EndHandlingComposition(nsIEditor* aEditor);

  





  class MOZ_STACK_CLASS CompositionChangeEventHandlingMarker
  {
  public:
    CompositionChangeEventHandlingMarker(
      TextComposition* aComposition,
      const WidgetCompositionEvent* aCompositionChangeEvent)
      : mComposition(aComposition)
    {
      mComposition->EditorWillHandleCompositionChangeEvent(
                      aCompositionChangeEvent);
    }

    ~CompositionChangeEventHandlingMarker()
    {
      mComposition->EditorDidHandleCompositionChangeEvent();
    }

  private:
    nsRefPtr<TextComposition> mComposition;
    CompositionChangeEventHandlingMarker();
    CompositionChangeEventHandlingMarker(
      const CompositionChangeEventHandlingMarker& aOther);
  };

private:
  
  ~TextComposition()
  {
    
  }

  
  
  
  
  nsPresContext* mPresContext;
  nsCOMPtr<nsINode> mNode;

  
  
  nsRefPtr<TextRangeArray> mRanges;

  
  
  void* mNativeContext;

  
  nsWeakPtr mEditorWeak;

  
  
  nsString mLastData;

  
  
  nsString mString;

  
  uint32_t mCompositionStartOffset;
  
  
  uint32_t mCompositionTargetOffset;

  
  bool mIsSynthesizedForTests;

  
  bool mIsComposing;

  
  
  bool mIsEditorHandlingEvent;

  
  
  
  bool mIsRequestingCommit;
  bool mIsRequestingCancel;

  
  
  
  
  
  bool mRequestedToCommitOrCancel;

  
  
  
  bool mWasNativeCompositionEndEventDiscarded;

  
  TextComposition() {}
  TextComposition(const TextComposition& aOther);

  


  already_AddRefed<nsIEditor> GetEditor() const;

  



  bool HasEditor() const;

  



  void EditorWillHandleCompositionChangeEvent(
         const WidgetCompositionEvent* aCompositionChangeEvent);

  



  void EditorDidHandleCompositionChangeEvent();

  





  bool IsValidStateForComposition(nsIWidget* aWidget) const;

  



  void DispatchCompositionEvent(WidgetCompositionEvent* aCompositionEvent,
                                nsEventStatus* aStatus,
                                EventDispatchingCallback* aCallBack,
                                bool aIsSynthesized);

  





  bool MaybeDispatchCompositionUpdate(
         const WidgetCompositionEvent* aCompositionEvent);

  





  BaseEventFlags CloneAndDispatchAs(
                   const WidgetCompositionEvent* aCompositionEvent,
                   uint32_t aMessage,
                   nsEventStatus* aStatus = nullptr,
                   EventDispatchingCallback* aCallBack = nullptr);

  



  bool WasNativeCompositionEndEventDiscarded() const
  {
    return mWasNativeCompositionEndEventDiscarded;
  }

  




  void OnCompositionEventDiscarded(
         const WidgetCompositionEvent* aCompositionEvent);

  


  void NotityUpdateComposition(const WidgetCompositionEvent* aCompositionEvent);

  



  class CompositionEventDispatcher : public nsRunnable
  {
  public:
    CompositionEventDispatcher(TextComposition* aTextComposition,
                               nsINode* aEventTarget,
                               uint32_t aEventMessage,
                               const nsAString& aData,
                               bool aIsSynthesizedEvent = false);
    NS_IMETHOD Run() override;

  private:
    nsRefPtr<TextComposition> mTextComposition;
    nsCOMPtr<nsINode> mEventTarget;
    uint32_t mEventMessage;
    nsString mData;
    bool mIsSynthesizedEvent;

    CompositionEventDispatcher() {};
  };

  












  void DispatchCompositionEventRunnable(uint32_t aEventMessage,
                                        const nsAString& aData,
                                        bool aIsSynthesizingCommit = false);
};











class TextCompositionArray final :
  public nsAutoTArray<nsRefPtr<TextComposition>, 2>
{
public:
  index_type IndexOf(nsIWidget* aWidget);
  index_type IndexOf(nsPresContext* aPresContext);
  index_type IndexOf(nsPresContext* aPresContext, nsINode* aNode);

  TextComposition* GetCompositionFor(nsIWidget* aWidget);
  TextComposition* GetCompositionFor(nsPresContext* aPresContext,
                                     nsINode* aNode);
  TextComposition* GetCompositionInContent(nsPresContext* aPresContext,
                                           nsIContent* aContent);
};

} 

#endif 
