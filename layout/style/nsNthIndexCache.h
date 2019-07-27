



#ifndef nsContentIndexCache_h__
#define nsContentIndexCache_h__

#include "js/HashTable.h"

class nsIContent;

namespace mozilla {
namespace dom {
class Element;
} 
} 






class nsNthIndexCache {
private:
  typedef mozilla::dom::Element Element;

public:
  



  nsNthIndexCache();
  ~nsNthIndexCache();

  
  
  
  
  
  
  
  int32_t GetNthIndex(Element* aChild, bool aIsOfType, bool aIsFromEnd,
                      bool aCheckEdgeOnly);

  void Reset();

private:
  



  inline bool SiblingMatchesElement(nsIContent* aSibling, Element* aElement,
                                    bool aIsOfType);

  
  
  
  
  typedef int32_t CacheEntry;

  class SystemAllocPolicy {
  public:
    void *malloc_(size_t bytes) { return ::malloc(bytes); }
    void *calloc_(size_t bytes) { return ::calloc(bytes, 1); }
    void *realloc_(void *p, size_t bytes) { return ::realloc(p, bytes); }
    void free_(void *p) { ::free(p); }
    void reportAllocOverflow() const {}
  };

  typedef js::HashMap<nsIContent*, CacheEntry, js::DefaultHasher<nsIContent*>,
                      SystemAllocPolicy> Cache;

  










  inline bool IndexDeterminedFromPreviousSibling(nsIContent* aSibling,
                                                 Element* aChild,
                                                 bool aIsOfType,
                                                 bool aIsFromEnd,
                                                 const Cache& aCache,
                                                 int32_t& aResult);

  
  
  
  
  
  Cache mCaches[2][2];
};

#endif 
