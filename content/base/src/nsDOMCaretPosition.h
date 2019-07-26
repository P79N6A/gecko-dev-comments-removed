



#ifndef nsDOMCaretPosition_h
#define nsDOMCaretPosition_h

#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsINode.h"
#include "nsWrapperCache.h"









class nsDOMCaretPosition : public nsISupports,
                           public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMCaretPosition)

  nsDOMCaretPosition(nsINode* aNode, uint32_t aOffset);

  





  uint32_t Offset() const { return mOffset; }

  








  nsINode* GetOffsetNode() const;

  nsISupports* GetParentObject() const
  {
    return GetOffsetNode();
  }

  virtual JSObject* WrapObject(JSContext *aCx, JSObject *aScope)
    MOZ_OVERRIDE MOZ_FINAL;

protected:
  virtual ~nsDOMCaretPosition();
  uint32_t mOffset;
  nsCOMPtr<nsINode> mOffsetNode;
};
#endif

