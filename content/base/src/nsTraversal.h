




 




#ifndef nsTraversal_h___
#define nsTraversal_h___

#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "mozilla/dom/CallbackObject.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/NodeFilterBinding.h"
#include "nsIDOMNodeFilter.h"

class nsINode;

class nsTraversal
{
public:
    nsTraversal(nsINode *aRoot,
                uint32_t aWhatToShow,
                const mozilla::dom::NodeFilterHolder &aFilter);
    virtual ~nsTraversal();

protected:
    nsCOMPtr<nsINode> mRoot;
    uint32_t mWhatToShow;
    mozilla::dom::NodeFilterHolder mFilter;
    bool mInAcceptNode;

    






    int16_t TestNode(nsINode* aNode, mozilla::ErrorResult& aResult);
};

#endif

