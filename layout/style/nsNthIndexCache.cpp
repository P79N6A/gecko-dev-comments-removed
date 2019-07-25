











































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
                                       bool aIsOfType)
{
  return aSibling->IsElement() &&
    (!aIsOfType ||
     aSibling->NodeInfo()->NameAndNamespaceEquals(aElement->NodeInfo()));
}

inline bool
nsNthIndexCache::IndexDeterminedFromPreviousSibling(nsIContent* aSibling,
                                                    Element* aChild,
                                                    bool aIsOfType,
                                                    bool aIsFromEnd,
                                                    PRInt32& aResult)
{
  if (SiblingMatchesElement(aSibling, aChild, aIsOfType)) {
    Cache::Ptr siblingEntry = mCache.lookup(aSibling);
    if (siblingEntry) {
      PRInt32 siblingIndex = siblingEntry->value.mNthIndices[aIsOfType][aIsFromEnd];
      NS_ASSERTION(siblingIndex != 0,
                   "How can a non-anonymous node have an anonymous sibling?");
      if (siblingIndex > 0) {
        
        
        
        
        
        NS_ABORT_IF_FALSE(aIsFromEnd == 0 || aIsFromEnd == 1,
                          "Bogus bool value");
        aResult = siblingIndex + aResult * (1 - 2 * aIsFromEnd);
        return true;
      }
    }
    
    ++aResult;
  }

  return false;
}

PRInt32
nsNthIndexCache::GetNthIndex(Element* aChild, bool aIsOfType,
                             bool aIsFromEnd, bool aCheckEdgeOnly)
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
  if (aCheckEdgeOnly) {
    
    
    if (aIsFromEnd) {
      for (nsIContent *cur = aChild->GetNextSibling();
           cur;
           cur = cur->GetNextSibling()) {
        if (SiblingMatchesElement(cur, aChild, aIsOfType)) {
          result = -1;
          break;
        }
      }
    } else {
      for (nsIContent *cur = aChild->GetPreviousSibling();
           cur;
           cur = cur->GetPreviousSibling()) {
        if (SiblingMatchesElement(cur, aChild, aIsOfType)) {
          result = -1;
          break;
        }
      }
    }
  } else {
    
    
    for (nsIContent *cur = aChild->GetPreviousSibling();
         cur;
         cur = cur->GetPreviousSibling()) {
      if (IndexDeterminedFromPreviousSibling(cur, aChild, aIsOfType,
                                             aIsFromEnd, result)) {
        slot = result;
        return result;
      }
    }

    
    
    
    
    
    if (aIsFromEnd) {
      result = 1;
      for (nsIContent *cur = aChild->GetNextSibling();
           cur;
           cur = cur->GetNextSibling()) {
        if (SiblingMatchesElement(cur, aChild, aIsOfType)) {
          ++result;
        }
      }
    }
  }

  slot = result;
  return result;
}
