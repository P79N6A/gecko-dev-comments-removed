









#include "nsNthIndexCache.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/NodeInfoInlines.h"

nsNthIndexCache::nsNthIndexCache()
{
}

nsNthIndexCache::~nsNthIndexCache()
{
}

void
nsNthIndexCache::Reset()
{
  mCaches[0][0].clear();
  mCaches[0][1].clear();
  mCaches[1][0].clear();
  mCaches[1][1].clear();
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
                                                    const Cache& aCache,
                                                    int32_t& aResult)
{
  if (SiblingMatchesElement(aSibling, aChild, aIsOfType)) {
    Cache::Ptr siblingEntry = aCache.lookup(aSibling);
    if (siblingEntry) {
      int32_t siblingIndex = siblingEntry->value();
      NS_ASSERTION(siblingIndex != 0,
                   "How can a non-anonymous node have an anonymous sibling?");
      if (siblingIndex > 0) {
        
        
        
        
        
        aResult = siblingIndex + aResult * (1 - 2 * aIsFromEnd);
        return true;
      }
    }

    ++aResult;
  }

  return false;
}

int32_t
nsNthIndexCache::GetNthIndex(Element* aChild, bool aIsOfType,
                             bool aIsFromEnd, bool aCheckEdgeOnly)
{
  NS_ASSERTION(aChild->GetParent(), "caller should check GetParent()");

  if (aChild->IsRootOfAnonymousSubtree()) {
    return 0;
  }

  Cache &cache = mCaches[aIsOfType][aIsFromEnd];

  if (!cache.initialized() && !cache.init()) {
    
    return 0;
  }

  Cache::AddPtr entry = cache.lookupForAdd(aChild);

  
  if (!entry && !cache.add(entry, aChild, -2)) {
    
    return 0;
  }

  int32_t &slot = entry->value();
  if (slot != -2 && (slot != -1 || aCheckEdgeOnly)) {
    return slot;
  }
  
  int32_t result = 1;
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
                                             aIsFromEnd, cache, result)) {
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
