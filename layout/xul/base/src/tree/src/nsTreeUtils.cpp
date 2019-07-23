







































#include "nsReadableUtils.h"
#include "nsTreeUtils.h"
#include "nsChildIterator.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsINodeInfo.h"

nsresult
nsTreeUtils::TokenizeProperties(const nsAString& aProperties, nsISupportsArray* aPropertiesArray)
{
  NS_PRECONDITION(aPropertiesArray != nsnull, "null ptr");
  if (! aPropertiesArray)
     return NS_ERROR_NULL_POINTER;

  nsAString::const_iterator end;
  aProperties.EndReading(end);

  nsAString::const_iterator iter;
  aProperties.BeginReading(iter);

  do {
    
    while (iter != end && nsCRT::IsAsciiSpace(*iter))
      ++iter;

    
    if (iter == end)
      break;

    
    nsAString::const_iterator first = iter;

    
    while (iter != end && ! nsCRT::IsAsciiSpace(*iter))
      ++iter;

    
    NS_ASSERTION(iter != first, "eh? something's wrong here");
    if (iter == first)
      break;

    nsCOMPtr<nsIAtom> atom = do_GetAtom(Substring(first, iter));
    aPropertiesArray->AppendElement(atom);
  } while (iter != end);

  return NS_OK;
}

nsresult
nsTreeUtils::GetImmediateChild(nsIContent* aContainer, nsIAtom* aTag,
                               nsIContent** aResult)
{
  ChildIterator iter, last;
  for (ChildIterator::Init(aContainer, &iter, &last); iter != last; ++iter) {
    nsCOMPtr<nsIContent> child = *iter;

    if (child->Tag() == aTag) {
      NS_ADDREF(*aResult = child);
      return NS_OK;
    }
  }

  *aResult = nsnull;
  return NS_OK;
}

nsresult
nsTreeUtils::GetDescendantChild(nsIContent* aContainer, nsIAtom* aTag, nsIContent** aResult)
{
  ChildIterator iter, last;
  for (ChildIterator::Init(aContainer, &iter, &last); iter != last; ++iter) {
    nsCOMPtr<nsIContent> child = *iter;
    if (child->Tag() == aTag) {
      NS_ADDREF(*aResult = child);
      return NS_OK;
    }
    else {
      nsresult rv = GetDescendantChild(child, aTag, aResult);
      if(NS_FAILED(rv))
        return rv;

      if(*aResult)
        return NS_OK;
    }
  }

  *aResult = nsnull;
  return NS_OK;
}

nsresult
nsTreeUtils::UpdateSortIndicators(nsIContent* aColumn, const nsAString& aDirection)
{
  aColumn->SetAttr(kNameSpaceID_None, nsGkAtoms::sortDirection, aDirection, PR_TRUE);
  aColumn->SetAttr(kNameSpaceID_None, nsGkAtoms::sortActive, NS_LITERAL_STRING("true"), PR_TRUE);

  
  nsCOMPtr<nsIContent> parentContent = aColumn->GetParent();
  if (parentContent &&
      parentContent->NodeInfo()->Equals(nsGkAtoms::treecols,
                                        kNameSpaceID_XUL)) {
    PRUint32 i, numChildren = parentContent->GetChildCount();
    for (i = 0; i < numChildren; ++i) {
      nsCOMPtr<nsIContent> childContent = parentContent->GetChildAt(i);

      if (childContent &&
          childContent != aColumn &&
          childContent->NodeInfo()->Equals(nsGkAtoms::treecol,
                                           kNameSpaceID_XUL)) {
        childContent->UnsetAttr(kNameSpaceID_None,
                                nsGkAtoms::sortDirection, PR_TRUE);
        childContent->UnsetAttr(kNameSpaceID_None,
                                nsGkAtoms::sortActive, PR_TRUE);
      }
    }
  }

  return NS_OK;
}

nsresult
nsTreeUtils::GetColumnIndex(nsIContent* aColumn, PRInt32* aResult)
{
  nsIContent* parentContent = aColumn->GetParent();
  if (parentContent &&
      parentContent->NodeInfo()->Equals(nsGkAtoms::treecols,
                                        kNameSpaceID_XUL)) {
    PRUint32 i, numChildren = parentContent->GetChildCount();
    PRInt32 colIndex = 0;
    for (i = 0; i < numChildren; ++i) {
      nsIContent *childContent = parentContent->GetChildAt(i);
      if (childContent &&
          childContent->NodeInfo()->Equals(nsGkAtoms::treecol,
                                           kNameSpaceID_XUL)) {
        if (childContent == aColumn) {
          *aResult = colIndex;
          return NS_OK;
        }
        ++colIndex;
      }
    }
  }

  *aResult = -1;
  return NS_OK;
}
