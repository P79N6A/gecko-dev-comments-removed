








#ifndef nsDOMAttributeMap_h
#define nsDOMAttributeMap_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Attr.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMMozNamedAttrMap.h"
#include "nsRefPtrHashtable.h"
#include "nsString.h"
#include "nsWrapperCache.h"

class nsIAtom;
class nsIDocument;




class nsAttrKey
{
public:
  


  int32_t  mNamespaceID;

  



  void* mLocalName;

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

  explicit nsAttrHashKey(KeyTypePointer aKey) : mKey(*aKey) {}
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


class nsDOMAttributeMap final : public nsIDOMMozNamedAttrMap
                              , public nsWrapperCache
{
public:
  typedef mozilla::dom::Attr Attr;
  typedef mozilla::dom::Element Element;
  typedef mozilla::ErrorResult ErrorResult;

  explicit nsDOMAttributeMap(Element *aContent);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsDOMAttributeMap)

  
  NS_DECL_NSIDOMMOZNAMEDATTRMAP

  void DropReference();

  Element* GetContent()
  {
    return mContent;
  }

  



  nsresult SetOwnerDocument(nsIDocument* aDocument);

  



  void DropAttribute(int32_t aNamespaceID, nsIAtom* aLocalName);

  






  uint32_t Count() const;

  typedef nsRefPtrHashtable<nsAttrHashKey, Attr> AttrCache;

  





  uint32_t Enumerate(AttrCache::EnumReadFunction aFunc, void *aUserArg) const;

  Element* GetParentObject() const
  {
    return mContent;
  }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  Attr* GetNamedItem(const nsAString& aAttrName);
  Attr* NamedGetter(const nsAString& aAttrName, bool& aFound);
  bool NameIsEnumerable(const nsAString& aName);
  already_AddRefed<Attr>
  SetNamedItem(Attr& aAttr, ErrorResult& aError)
  {
    return SetNamedItemInternal(aAttr, false, aError);
  }
  already_AddRefed<Attr>
  RemoveNamedItem(mozilla::dom::NodeInfo* aNodeInfo, ErrorResult& aError);
  already_AddRefed<Attr>
  RemoveNamedItem(const nsAString& aName, ErrorResult& aError);
 
  Attr* Item(uint32_t aIndex);
  Attr* IndexedGetter(uint32_t aIndex, bool& aFound);
  uint32_t Length() const;

  Attr*
  GetNamedItemNS(const nsAString& aNamespaceURI,
                 const nsAString& aLocalName);
  already_AddRefed<Attr>
  SetNamedItemNS(Attr& aNode, ErrorResult& aError)
  {
    return SetNamedItemInternal(aNode, true, aError);
  }
  already_AddRefed<Attr>
  RemoveNamedItemNS(const nsAString& aNamespaceURI, const nsAString& aLocalName,
                    ErrorResult& aError);

  void GetSupportedNames(unsigned, nsTArray<nsString>& aNames)
  {
    
  }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
  virtual ~nsDOMAttributeMap();

private:
  nsCOMPtr<Element> mContent;

  


  AttrCache mAttributeCache;

  



  already_AddRefed<Attr>
  SetNamedItemInternal(Attr& aNode, bool aWithNS, ErrorResult& aError);

  already_AddRefed<mozilla::dom::NodeInfo>
  GetAttrNodeInfo(const nsAString& aNamespaceURI,
                  const nsAString& aLocalName);

  Attr* GetAttribute(mozilla::dom::NodeInfo* aNodeInfo, bool aNsAware);

  


  already_AddRefed<Attr> RemoveAttribute(mozilla::dom::NodeInfo* aNodeInfo);
};




#include "mozilla/dom/Element.h"

#endif 
