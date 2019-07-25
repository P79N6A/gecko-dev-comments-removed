






































 




#ifndef nsTreeWalker_h___
#define nsTreeWalker_h___

#include "nsIDOMTreeWalker.h"
#include "nsTraversal.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"

class nsINode;
class nsIDOMNode;
class nsIDOMNodeFilter;

class nsTreeWalker : public nsIDOMTreeWalker, public nsTraversal
{
public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIDOMTREEWALKER

    nsTreeWalker(nsINode *aRoot,
                 PRUint32 aWhatToShow,
                 nsIDOMNodeFilter *aFilter,
                 PRBool aExpandEntityReferences);
    virtual ~nsTreeWalker();

    NS_DECL_CYCLE_COLLECTION_CLASS(nsTreeWalker)

private:
    nsCOMPtr<nsINode> mCurrentNode;

    






    nsresult FirstChildInternal(PRBool aReversed, nsIDOMNode **_retval);

    






    nsresult NextSiblingInternal(PRBool aReversed, nsIDOMNode **_retval);
};

#endif

