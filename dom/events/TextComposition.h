





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







class TextComposition MOZ_FINAL
{
  friend class IMEStateManager;

  NS_INLINE_DECL_REFCOUNTING(TextComposition)

public:
  TextComposition(nsPresContext* aPresContext,
                  nsINode* aNode,
                  WidgetGUIEvent* aEvent);

  bool Destroyed() const { return !mPresContext; }
  nsPresContext* GetPresContext() const { return mPresContext; }
  nsINode* GetEventTargetNode() const { return mNode; }
  
  
  const nsString& LastData() const { return mLastData; }
  
  
  
  
  
  const nsString& String() const { return mString; }
  
  
  
  
  
  TextRangeArray* GetRanges() const { return mRanges; }
  
  
  bool IsSynthesizedForTests() const { return mIsSynthesizedForTests; }

  bool MatchesNativeContext(nsIWidget* aWidget) const;

  


  void Destroy();

  





  void SynthesizeCommit(bool aDiscard);

  



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

  




  class MOZ_STACK_CLASS TextEventHandlingMarker
  {
  public:
    TextEventHandlingMarker(TextComposition* aComposition,
                            const WidgetTextEvent* aTextEvent)
      : mComposition(aComposition)
    {
      mComposition->EditorWillHandleTextEvent(aTextEvent);
    }

    ~TextEventHandlingMarker()
    {
      mComposition->EditorDidHandleTextEvent();
    }

  private:
    nsRefPtr<TextComposition> mComposition;
    TextEventHandlingMarker();
    TextEventHandlingMarker(const TextEventHandlingMarker& aOther);
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

  
  TextComposition() {}
  TextComposition(const TextComposition& aOther);

  


  already_AddRefed<nsIEditor> GetEditor() const;

  



  bool HasEditor() const;

  



  void EditorWillHandleTextEvent(const WidgetTextEvent* aTextEvent);

  



  void EditorDidHandleTextEvent();

  



  void DispatchEvent(WidgetGUIEvent* aEvent,
                     nsEventStatus* aStatus,
                     EventDispatchingCallback* aCallBack);

  


  void NotityUpdateComposition(WidgetGUIEvent* aEvent);

  



  class CompositionEventDispatcher : public nsRunnable
  {
  public:
    CompositionEventDispatcher(nsPresContext* aPresContext,
                               nsINode* aEventTarget,
                               uint32_t aEventMessage,
                               const nsAString& aData);
    NS_IMETHOD Run() MOZ_OVERRIDE;

  private:
    nsRefPtr<nsPresContext> mPresContext;
    nsCOMPtr<nsINode> mEventTarget;
    nsCOMPtr<nsIWidget> mWidget;
    uint32_t mEventMessage;
    nsString mData;

    CompositionEventDispatcher() {};
  };

  












  void DispatchCompositionEventRunnable(uint32_t aEventMessage,
                                        const nsAString& aData);
};











class TextCompositionArray MOZ_FINAL :
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
