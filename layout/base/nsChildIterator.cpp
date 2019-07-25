










































#include "nsChildIterator.h"
#include "nsIDocument.h"
#include "nsBindingManager.h"

nsresult
ChildIterator::Init(nsIContent*    aContent,
                    ChildIterator* aFirst,
                    ChildIterator* aLast)
{
  
  aFirst->mContent = aLast->mContent = nsnull;
  aFirst->mChild   = aLast->mChild   = nsnull;
  
  NS_PRECONDITION(aContent != nsnull, "no content");
  if (! aContent)
    return NS_ERROR_NULL_POINTER;

  nsIDocument* doc = aContent->OwnerDoc();

  
  
  nsINodeList* nodes = doc->BindingManager()->GetXBLChildNodesFor(aContent);

  aFirst->mContent = aContent;
  aLast->mContent  = aContent;
  aFirst->mNodes   = nodes;
  aLast->mNodes    = nodes;

  if (nodes) {
    PRUint32 length;
    nodes->GetLength(&length);
    aFirst->mIndex = 0;
    aLast->mIndex = length;
  } else {
    aFirst->mChild = aContent->GetFirstChild();
    aLast->mChild = nsnull;
  }

  return NS_OK;
}
