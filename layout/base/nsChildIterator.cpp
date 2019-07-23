










































#include "nsChildIterator.h"
#include "nsIDocument.h"
#include "nsBindingManager.h"

nsresult
ChildIterator::Init(nsIContent*    aContent,
                    ChildIterator* aFirst,
                    ChildIterator* aLast)
{
  
  aFirst->mContent = aLast->mContent = nsnull;
  aFirst->mIndex   = aLast->mIndex   = 0;
  
  NS_PRECONDITION(aContent != nsnull, "no content");
  if (! aContent)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDocument> doc = aContent->GetDocument();
  NS_ASSERTION(doc, "element not in the document");
  if (! doc)
    return NS_ERROR_FAILURE;

  
  
  nsCOMPtr<nsIDOMNodeList> nodes;
  doc->BindingManager()->GetXBLChildNodesFor(aContent, getter_AddRefs(nodes));

  PRUint32 length;
  if (nodes)
    nodes->GetLength(&length);
  else
    length = aContent->GetChildCount();

  aFirst->mContent = aContent;
  aLast->mContent  = aContent;
  aFirst->mIndex   = 0;
  aLast->mIndex    = length;
  aFirst->mNodes   = nodes;
  aLast->mNodes    = nodes;

  return NS_OK;
}
