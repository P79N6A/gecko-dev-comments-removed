








#ifndef nsDOMAttributeMap_h
#define nsDOMAttributeMap_h

#include "nsIDOMMozNamedAttrMap.h"
#include "nsStringGlue.h"
#include "nsRefPtrHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMAttr.h"
#include "mozilla/ErrorResult.h"

class nsIAtom;
class nsDOMAttribute;
class nsINodeInfo;
class nsIDocument;

namespace mozilla {
namespace dom {
class Element;
} 
} 




class nsAttrKey
{
public:
  


  int32_t  mNamespaceID;

  



  nsIAtom* mLocalName;

  nsAttrKey(int32_t aNs, nsIAtom* aName)
    : mNamespaceID(aNs), mLocalName(aName) {}

  nsAttrKey(const nsAttrKey& aAttr)
    : mNamespaceID(aAttr.mNamespaceID), mLocalName(aAttr.mLocalName) {}
};




class nsAttrHashKey : public PLDHashEntryHdr
{
public:
  typedef const nsAttrKey& KeyType;
  typedef const nsAttrKey* KeyTypePointer;

  nsAttrHashKey(KeyTypePointer aKey) : mKey(*aKey) {}
  nsAttrHashKey(const nsAttrHashKey& aCopy) : mKey(aCopy.mKey) {}
  ~nsAttrHashKey() {}

  KeyType GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const
    {
      return mKey.mLocalName == aKey->mLocalName &&
             mKey.mNamespaceID == aKey->mNamespaceID;
    }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      if (!aKey)
        return 0;

      return mozilla::HashGeneric(aKey->mNamespaceID, aKey->mLocalName);
    }
  enum { ALLOW_MEMMOVE = true };

private:
  nsAttrKey mKey;
};


class nsDOMAttributeMap : public nsIDOMMozNamedAttrMap
{
public:
  typedef mozilla::dom::Element Element;

  nsDOMAttributeMap(Element *aContent);
  virtual ~nsDOMAttributeMap();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMMOZNAMEDATTRMAP

  void DropReference();

  Element* GetContent()
  {
    return mContent;
  }

  



  nsresult SetOwnerDocument(nsIDocument* aDocument);

  



  void DropAttribute(int32_t aNamespaceID, nsIAtom* aLocalName);

  






  uint32_t Count() const;

  typedef nsRefPtrHashtable<nsAttrHashKey, nsDOMAttribute> AttrCache;

  





  uint32_t Enumerate(AttrCache::EnumReadFunction aFunc, void *aUserArg) const;

  nsDOMAttribute* GetItemAt(uint32_t aIndex, nsresult *rv);
  nsDOMAttribute* GetNamedItem(const nsAString& aAttrName);

  static nsDOMAttributeMap* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMMozNamedAttrMap> map_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(map_qi == static_cast<nsIDOMMozNamedAttrMap*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMAttributeMap*>(aSupports);
  }

  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMAttributeMap)

  nsDOMAttribute* GetNamedItemNS(const nsAString& aNamespaceURI,
                                 const nsAString& aLocalName,
                                 mozilla::ErrorResult& aError);

  already_AddRefed<nsDOMAttribute> SetNamedItemNS(nsIDOMAttr *aNode,
                                                  mozilla::ErrorResult& aError)
  {
    return SetNamedItemInternal(aNode, true, aError);
  }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
  Element *mContent; 

  


  AttrCache mAttributeCache;

  



  already_AddRefed<nsDOMAttribute>
    SetNamedItemInternal(nsIDOMAttr *aNode,
                         bool aWithNS,
                         mozilla::ErrorResult& aError);

  already_AddRefed<nsINodeInfo>
  GetAttrNodeInfo(const nsAString& aNamespaceURI,
                  const nsAString& aLocalName,
                  mozilla::ErrorResult& aError);

  nsDOMAttribute* GetAttribute(nsINodeInfo* aNodeInfo, bool aNsAware);

  


  already_AddRefed<nsDOMAttribute> RemoveAttribute(nsINodeInfo* aNodeInfo);
};


#endif 
