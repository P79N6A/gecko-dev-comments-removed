




 




#ifndef mozilla_dom_TreeWalker_h
#define mozilla_dom_TreeWalker_h

#include "nsIDOMTreeWalker.h"
#include "nsTraversal.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"

class nsINode;
class nsIDOMNode;

namespace mozilla {
namespace dom {

class TreeWalker final : public nsIDOMTreeWalker, public nsTraversal
{
    virtual ~TreeWalker();

public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDOMTREEWALKER

    TreeWalker(nsINode *aRoot,
               uint32_t aWhatToShow,
               const NodeFilterHolder &aFilter);

    NS_DECL_CYCLE_COLLECTION_CLASS(TreeWalker)

    
    nsINode* Root() const
    {
        return mRoot;
    }
    uint32_t WhatToShow() const
    {
        return mWhatToShow;
    }
    already_AddRefed<NodeFilter> GetFilter()
    {
        return mFilter.ToWebIDLCallback();
    }
    nsINode* CurrentNode() const
    {
        return mCurrentNode;
    }
    void SetCurrentNode(nsINode& aNode, ErrorResult& aResult);
    
    
    already_AddRefed<nsINode> ParentNode(ErrorResult& aResult);
    already_AddRefed<nsINode> FirstChild(ErrorResult& aResult);
    already_AddRefed<nsINode> LastChild(ErrorResult& aResult);
    already_AddRefed<nsINode> PreviousSibling(ErrorResult& aResult);
    already_AddRefed<nsINode> NextSibling(ErrorResult& aResult);
    already_AddRefed<nsINode> PreviousNode(ErrorResult& aResult);
    already_AddRefed<nsINode> NextNode(ErrorResult& aResult);

    bool WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto, JS::MutableHandle<JSObject*> aReflector);

private:
    nsCOMPtr<nsINode> mCurrentNode;

    






    already_AddRefed<nsINode> FirstChildInternal(bool aReversed,
                                                 ErrorResult& aResult);

    






    already_AddRefed<nsINode> NextSiblingInternal(bool aReversed,
                                                  ErrorResult& aResult);

    
    typedef already_AddRefed<nsINode> (TreeWalker::*NodeGetter)(ErrorResult&);
    inline nsresult ImplNodeGetter(NodeGetter aGetter, nsIDOMNode** aRetval)
    {
        mozilla::ErrorResult rv;
        nsCOMPtr<nsINode> node = (this->*aGetter)(rv);
        if (rv.Failed()) {
            return rv.StealNSResult();
        }
        *aRetval = node ? node.forget().take()->AsDOMNode() : nullptr;
        return NS_OK;
    }
};

} 
} 

#endif 

