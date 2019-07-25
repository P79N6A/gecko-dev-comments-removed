





































#ifndef nsContentIndexCache_h__
#define nsContentIndexCache_h__

#include "nscore.h"
#include "js/HashTable.h"
#include "mozilla/dom/Element.h"






class nsNthIndexCache {
private:
  typedef mozilla::dom::Element Element;

public:
  



  nsNthIndexCache();
  ~nsNthIndexCache();

  
  
  
  
  
  
  
  PRInt32 GetNthIndex(const Element* aChild, bool aIsOfType, bool aIsFromEnd,
                      bool aCheckEdgeOnly);

  void Reset();

private:
  



  inline bool SiblingMatchesElement(const nsIContent* aSibling, 
                                    const Element* aElement,
                                    bool aIsOfType);

  
  
  
  
  typedef PRInt32 CacheEntry;

  class SystemAllocPolicy {
  public:
    void *malloc_(size_t bytes) { return ::malloc(bytes); }
    void *realloc_(void *p, size_t bytes) { return ::realloc(p, bytes); }
    void free_(void *p) { ::free(p); }
    void reportAllocOverflow() const {}
  };

  typedef js::HashMap<const nsIContent*, CacheEntry,
                      js::DefaultHasher<const nsIContent*>,
                      SystemAllocPolicy> Cache;

  










  inline bool IndexDeterminedFromPreviousSibling(const nsIContent* aSibling,
                                                 const Element* aChild,
                                                 bool aIsOfType,
                                                 bool aIsFromEnd,
                                                 const Cache& aCache,
                                                 PRInt32& aResult);

  
  
  
  
  
  Cache mCaches[2][2];
};

#endif 
