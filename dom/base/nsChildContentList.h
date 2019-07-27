





#ifndef nsChildContentList_h__
#define nsChildContentList_h__

#include "nsISupportsImpl.h"
#include "nsINodeList.h"            
#include "js/TypeDecls.h"     

class nsIContent;
class nsINode;







class nsChildContentList final : public nsINodeList
{
public:
  explicit nsChildContentList(nsINode* aNode)
    : mNode(aNode)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsChildContentList)

  
  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent) override;
  virtual nsIContent* Item(uint32_t aIndex) override;

  void DropReference()
  {
    mNode = nullptr;
  }

  virtual nsINode* GetParentObject() override
  {
    return mNode;
  }

private:
  ~nsChildContentList() {}

  
  
  
  nsINode* MOZ_NON_OWNING_REF mNode;
};

#endif 
