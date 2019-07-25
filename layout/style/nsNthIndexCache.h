





































#ifndef nsContentIndexCache_h__
#define nsContentIndexCache_h__

#include "nscore.h"
#include "jshashtable.h"
#include "mozilla/dom/Element.h"






class nsNthIndexCache {
private:
  typedef mozilla::dom::Element Element;

public:
  



  nsNthIndexCache();
  ~nsNthIndexCache();

  
  
  
  
  
  
  
  PRInt32 GetNthIndex(Element* aChild, bool aIsOfType, bool aIsFromEnd,
                      bool aCheckEdgeOnly);

  void Reset();

private:
  



  inline bool SiblingMatchesElement(nsIContent* aSibling, Element* aElement,
                                    bool aIsOfType);

  



  inline bool IndexDetermined(nsIContent* aSibling, Element* aChild,
                              bool aIsOfType, bool aIsFromEnd,
                              bool aCheckEdgeOnly, PRInt32& aResult);

  struct CacheEntry {
    CacheEntry() {
      mNthIndices[0][0] = -2;
      mNthIndices[0][1] = -2;
      mNthIndices[1][0] = -2;
      mNthIndices[1][1] = -2;
    }

    
    
    
    
    
    
    PRInt32 mNthIndices[2][2];
  };

  class SystemAllocPolicy {
  public:
    void *malloc_(size_t bytes) { return ::malloc(bytes); }
    void *realloc_(void *p, size_t bytes) { return ::realloc(p, bytes); }
    void free_(void *p) { ::free(p); }
    void reportAllocOverflow() const {}
  };

  typedef js::HashMap<nsIContent*, CacheEntry, js::DefaultHasher<nsIContent*>,
                      SystemAllocPolicy> Cache;

  Cache mCache;
};

#endif 
