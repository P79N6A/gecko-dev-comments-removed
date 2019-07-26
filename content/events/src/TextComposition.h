





#ifndef mozilla_TextComposition_h
#define mozilla_TextComposition_h

#include "nsCOMPtr.h"
#include "nsEvent.h"
#include "nsINode.h"
#include "nsIWidget.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"

class nsDispatchingCallback;
class nsIMEStateManager;
class nsIWidget;

namespace mozilla {







class TextComposition MOZ_FINAL
{
  friend class ::nsIMEStateManager;
public:
  TextComposition(nsPresContext* aPresContext,
                  nsINode* aNode,
                  nsGUIEvent* aEvent);

  TextComposition(const TextComposition& aOther);

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

private:
  
  
  
  
  nsPresContext* mPresContext;
  nsCOMPtr<nsINode> mNode;

  
  
  void* mNativeContext;

  
  
  nsString mLastData;

  
  bool mIsSynthesizedForTests;

  
  TextComposition() {}

  



  void DispatchEvent(nsGUIEvent* aEvent,
                     nsEventStatus* aStatus,
                     nsDispatchingCallback* aCallBack);

  



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











class TextCompositionArray MOZ_FINAL : public nsAutoTArray<TextComposition, 2>
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
