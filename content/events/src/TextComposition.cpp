





#include "TextComposition.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsGUIEvent.h"
#include "nsIPresShell.h"
#include "nsIWidget.h"
#include "nsPresContext.h"

namespace mozilla {





TextComposition::TextComposition(nsPresContext* aPresContext,
                                 nsINode* aNode,
                                 nsGUIEvent* aEvent) :
  mPresContext(aPresContext), mNode(aNode),
  
  
  mNativeContext(aEvent->widget)
{
}

TextComposition::TextComposition(const TextComposition& aOther)
{
  mNativeContext = aOther.mNativeContext;
  mPresContext = aOther.mPresContext;
  mNode = aOther.mNode;
}

bool
TextComposition::MatchesNativeContext(nsIWidget* aWidget) const
{
  
  
  return mNativeContext == static_cast<void*>(aWidget);
}

bool
TextComposition::MatchesEventTarget(nsPresContext* aPresContext,
                                    nsINode* aNode) const
{
  return mPresContext == aPresContext && mNode == aNode;
}

void
TextComposition::DispatchEvent(nsGUIEvent* aEvent,
                               nsEventStatus* aStatus,
                               nsDispatchingCallback* aCallBack)
{
  nsEventDispatcher::Dispatch(mNode, mPresContext,
                              aEvent, nullptr, aStatus, aCallBack);
}





TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsIWidget* aWidget)
{
  for (index_type i = Length(); i > 0; --i) {
    if (ElementAt(i - 1).MatchesNativeContext(aWidget)) {
      return i - 1;
    }
  }
  return NoIndex;
}

TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsPresContext* aPresContext)
{
  for (index_type i = Length(); i > 0; --i) {
    if (ElementAt(i - 1).GetPresContext() == aPresContext) {
      return i - 1;
    }
  }
  return NoIndex;
}

TextCompositionArray::index_type
TextCompositionArray::IndexOf(nsPresContext* aPresContext,
                              nsINode* aNode)
{
  index_type index = IndexOf(aPresContext);
  if (index == NoIndex) {
    return NoIndex;
  }
  nsINode* node = ElementAt(index).GetEventTargetNode();
  return node == aNode ? index : NoIndex;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsIWidget* aWidget)
{
  index_type i = IndexOf(aWidget);
  return i != NoIndex ? &ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsPresContext* aPresContext)
{
  index_type i = IndexOf(aPresContext);
  return i != NoIndex ? &ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionFor(nsPresContext* aPresContext,
                                           nsINode* aNode)
{
  index_type i = IndexOf(aPresContext, aNode);
  return i != NoIndex ? &ElementAt(i) : nullptr;
}

TextComposition*
TextCompositionArray::GetCompositionInContent(nsPresContext* aPresContext,
                                              nsIContent* aContent)
{
  
  for (index_type i = Length(); i > 0; --i) {
    nsINode* node = ElementAt(i - 1).GetEventTargetNode();
    if (node && nsContentUtils::ContentIsDescendantOf(node, aContent)) {
      return &ElementAt(i - 1);
    }
  }
  return nullptr;
}

} 
