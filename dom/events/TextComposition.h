





#ifndef mozilla_TextComposition_h
#define mozilla_TextComposition_h

#include "nsCOMPtr.h"
#include "nsINode.h"
#include "nsIWidget.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsPresContext.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"

class nsDispatchingCallback;
class nsIMEStateManager;
class nsIWidget;

namespace mozilla {







class TextComposition MOZ_FINAL
{
  friend class ::nsIMEStateManager;

  NS_INLINE_DECL_REFCOUNTING(TextComposition)

public:
  TextComposition(nsPresContext* aPresContext,
                  nsINode* aNode,
                  WidgetGUIEvent* aEvent);

  ~TextComposition()
  {
    
  }

  nsPresContext* GetPresContext() const { return mPresContext; }
  nsINode* GetEventTargetNode() const { return mNode; }
  
  const nsString& GetLastData() const { return mLastData; }
  
  
  bool IsSynthesizedForTests() const { return mIsSynthesizedForTests; }

  bool MatchesNativeContext(nsIWidget* aWidget) const;

  





  void SynthesizeCommit(bool aDiscard);

  



  nsresult NotifyIME(widget::NotificationToIME aNotification);

  


  uint32_t OffsetOfTargetClause() const { return mCompositionTargetOffset; }

  



  bool IsComposing() const { return mIsComposing; }

  



  void EditorWillHandleTextEvent(const WidgetTextEvent* aTextEvent);

private:
  
  
  
  
  nsPresContext* mPresContext;
  nsCOMPtr<nsINode> mNode;

  
  
  void* mNativeContext;

  
  
  nsString mLastData;

  
  uint32_t mCompositionStartOffset;
  
  
  uint32_t mCompositionTargetOffset;

  
  bool mIsSynthesizedForTests;

  
  bool mIsComposing;

  
  TextComposition() {}
  TextComposition(const TextComposition& aOther);


  



  void DispatchEvent(WidgetGUIEvent* aEvent,
                     nsEventStatus* aStatus,
                     nsDispatchingCallback* aCallBack);

  


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

  












  void DispatchCompsotionEventRunnable(uint32_t aEventMessage,
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
