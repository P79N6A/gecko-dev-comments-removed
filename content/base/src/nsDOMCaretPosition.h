



#ifndef nsDOMCaretPosition_h
#define nsDOMCaretPosition_h

#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsINode.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {
class DOMRect;
}
}









class nsDOMCaretPosition : public nsISupports,
                           public nsWrapperCache
{
  typedef mozilla::dom::DOMRect DOMRect;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMCaretPosition)

  nsDOMCaretPosition(nsINode* aNode, uint32_t aOffset);

  





  uint32_t Offset() const { return mOffset; }

  








  nsINode* GetOffsetNode() const;

  






  already_AddRefed<DOMRect> GetClientRect() const;

  











  void SetAnonymousContentNode(nsINode* aNode)
  {
    mAnonymousContentNode = aNode;
  }

  nsISupports* GetParentObject() const
  {
    return GetOffsetNode();
  }

  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aScope)
    MOZ_OVERRIDE MOZ_FINAL;

protected:
  virtual ~nsDOMCaretPosition();

  uint32_t mOffset;
  nsCOMPtr<nsINode> mOffsetNode;
  nsCOMPtr<nsINode> mAnonymousContentNode;
};
#endif

