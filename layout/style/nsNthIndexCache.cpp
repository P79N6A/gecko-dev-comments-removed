











































#include "nsNthIndexCache.h"
#include "nsIContent.h"

nsNthIndexCache::nsNthIndexCache()
{
}

nsNthIndexCache::~nsNthIndexCache()
{
}

void
nsNthIndexCache::Reset()
{
  mCache.clear();
}

inline bool
nsNthIndexCache::SiblingMatchesElement(nsIContent* aSibling, Element* aElement,
                                       PRBool aIsOfType)
{
  return aSibling->IsElement() &&
    (!aIsOfType ||
     aSibling->NodeInfo()->NameAndNamespaceEquals(aElement->NodeInfo()));
}

inline bool
nsNthIndexCache::IndexDetermined(nsIContent* aSibling, Element* aChild,
                                 PRBool aIsOfType, PRBool aIsFromEnd,
                                 PRBool aCheckEdgeOnly, PRInt32& aResult)
{
  if (SiblingMatchesElement(aSibling, aChild, aIsOfType)) {
    if (aCheckEdgeOnly) {
      
      
      aResult = -1;
      return true;
    }

    Cache::Ptr siblingEntry = mCache.lookup(aSibling);
    if (siblingEntry) {
      PRInt32 siblingIndex = siblingEntry->value.mNthIndices[aIsOfType][aIsFromEnd];
      NS_ASSERTION(siblingIndex != 0,
                   "How can a non-anonymous node have an anonymous sibling?");
      if (siblingIndex > 0) {
        
        
        
        
        aResult = siblingIndex + aResult;
        return true;
      }
    }
    
    ++aResult;
  }

  return false;
}

PRInt32
nsNthIndexCache::GetNthIndex(Element* aChild, PRBool aIsOfType,
                             PRBool aIsFromEnd, PRBool aCheckEdgeOnly)
{
  NS_ASSERTION(aChild->GetParent(), "caller should check GetParent()");

  if (aChild->IsRootOfAnonymousSubtree()) {
    return 0;
  }

  if (!mCache.initialized() && !mCache.init()) {
    
    return 0;
  }

  Cache::AddPtr entry = mCache.lookupForAdd(aChild);
  
  if (!entry && !mCache.add(entry, aChild)) {
    
    return 0;
  }

  PRInt32 &slot = entry->value.mNthIndices[aIsOfType][aIsFromEnd];
  if (slot != -2 && (slot != -1 || aCheckEdgeOnly)) {
    return slot;
  }
  
  PRInt32 result = 1;
  if (aIsFromEnd) {
    for (nsIContent *cur = aChild->GetNextSibling();
         cur;
         cur = cur->GetNextSibling()) {
      
      
      
      if (SiblingMatchesElement(cur, aChild, aIsOfType)) {
        if (aCheckEdgeOnly) {
          
          
          result = -1;
          break;
        }
        ++result;
      }
    }
  } else {
    for (nsIContent *cur = aChild->GetPreviousSibling();
         cur;
         cur = cur->GetPreviousSibling()) {
      if (IndexDetermined(cur, aChild, aIsOfType, aIsFromEnd, aCheckEdgeOnly,
                          result)) {
        break;
      }
    }
  }

  slot = result;
  return result;
}
