










#include "nsChildIterator.h"
#include "nsIDocument.h"
#include "nsBindingManager.h"

nsresult
ChildIterator::Init(nsIContent*    aContent,
                    ChildIterator* aFirst,
                    ChildIterator* aLast)
{
  
  aFirst->mContent = aLast->mContent = nullptr;
  aFirst->mChild   = aLast->mChild   = nullptr;
  
  NS_PRECONDITION(aContent != nullptr, "no content");
  if (! aContent)
    return NS_ERROR_NULL_POINTER;

  nsIDocument* doc = aContent->OwnerDoc();

  
  
  nsINodeList* nodes = doc->BindingManager()->GetXBLChildNodesFor(aContent);

  aFirst->mContent = aContent;
  aLast->mContent  = aContent;
  aFirst->mNodes   = nodes;
  aLast->mNodes    = nodes;

  if (nodes) {
    uint32_t length;
    nodes->GetLength(&length);
    aFirst->mIndex = 0;
    aLast->mIndex = length;
  } else {
    aFirst->mChild = aContent->GetFirstChild();
    aLast->mChild = nullptr;
  }

  return NS_OK;
}
