




 




#ifndef nsTraversal_h___
#define nsTraversal_h___

#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "mozilla/dom/CallbackObject.h"

class nsINode;
class nsIDOMNodeFilter;

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

    






    nsresult TestNode(nsINode* aNode, int16_t* _filtered);
};

#endif

