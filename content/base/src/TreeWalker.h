




 




#ifndef mozilla_dom_TreeWalker_h
#define mozilla_dom_TreeWalker_h

#include "nsIDOMTreeWalker.h"
#include "nsTraversal.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"

class nsINode;
class nsIDOMNode;
class nsIDOMNodeFilter;

namespace mozilla {
namespace dom {

class TreeWalker : public nsIDOMTreeWalker, public nsTraversal
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDOMTREEWALKER

    TreeWalker(nsINode *aRoot,
               uint32_t aWhatToShow,
               const NodeFilterHolder &aFilter);
    virtual ~TreeWalker();

    NS_DECL_CYCLE_COLLECTION_CLASS(TreeWalker)

private:
    nsCOMPtr<nsINode> mCurrentNode;

    






    nsresult FirstChildInternal(bool aReversed, nsIDOMNode **_retval);

    






    nsresult NextSiblingInternal(bool aReversed, nsIDOMNode **_retval);
};

} 
} 

#endif 

