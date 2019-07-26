





#ifndef mozilla_TextComposition_h
#define mozilla_TextComposition_h

#include "nsCOMPtr.h"
#include "nsEvent.h"
#include "nsINode.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"

class nsCompositionEvent;
class nsDispatchingCallback;
class nsIMEStateManager;
class nsIWidget;
class nsPresContext;

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

  bool MatchesNativeContext(nsIWidget* aWidget) const;
  bool MatchesEventTarget(nsPresContext* aPresContext,
                          nsINode* aNode) const;

private:
  
  
  
  
  nsPresContext* mPresContext;
  nsCOMPtr<nsINode> mNode;

  
  
  void* mNativeContext;

  
  TextComposition() {}

  



  void DispatchEvent(nsGUIEvent* aEvent,
                     nsEventStatus* aStatus,
                     nsDispatchingCallback* aCallBack);
};











class TextCompositionArray MOZ_FINAL : public nsAutoTArray<TextComposition, 2>
{
public:
  index_type IndexOf(nsIWidget* aWidget);
  index_type IndexOf(nsPresContext* aPresContext);
  index_type IndexOf(nsPresContext* aPresContext, nsINode* aNode);

  TextComposition* GetCompositionFor(nsIWidget* aWidget);
  TextComposition* GetCompositionFor(nsPresContext* aPresContext);
  TextComposition* GetCompositionFor(nsPresContext* aPresContext,
                                     nsINode* aNode);
  TextComposition* GetCompositionInContent(nsPresContext* aPresContext,
                                           nsIContent* aContent);
};

} 

#endif 
