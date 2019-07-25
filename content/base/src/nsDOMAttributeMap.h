









































#ifndef nsDOMAttributeMap_h___
#define nsDOMAttributeMap_h___

#include "nsIDOMNamedNodeMap.h"
#include "nsString.h"
#include "nsRefPtrHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "prbit.h"
#include "nsIDOMNode.h"

class nsIAtom;
class nsIContent;
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
  


  PRInt32  mNamespaceID;

  



  nsIAtom* mLocalName;

  nsAttrKey(PRInt32 aNs, nsIAtom* aName)
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

      return PR_ROTATE_LEFT32(static_cast<PRUint32>(aKey->mNamespaceID), 4) ^
             NS_PTR_TO_INT32(aKey->mLocalName);
    }
  enum { ALLOW_MEMMOVE = PR_TRUE };

private:
  nsAttrKey mKey;
};


class nsDOMAttributeMap : public nsIDOMNamedNodeMap
{
public:
  typedef mozilla::dom::Element Element;

  nsDOMAttributeMap(Element *aContent);
  virtual ~nsDOMAttributeMap();

  


  bool Init();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMNAMEDNODEMAP

  void DropReference();

  Element* GetContent()
  {
    return mContent;
  }

  



  nsresult SetOwnerDocument(nsIDocument* aDocument);

  



  void DropAttribute(PRInt32 aNamespaceID, nsIAtom* aLocalName);

  






  PRUint32 Count() const;

  typedef nsRefPtrHashtable<nsAttrHashKey, nsDOMAttribute> AttrCache;

  





  PRUint32 Enumerate(AttrCache::EnumReadFunction aFunc, void *aUserArg) const;

  nsDOMAttribute* GetItemAt(PRUint32 aIndex, nsresult *rv);
  nsDOMAttribute* GetNamedItem(const nsAString& aAttrName, nsresult *rv);

  static nsDOMAttributeMap* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMNamedNodeMap> map_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(map_qi == static_cast<nsIDOMNamedNodeMap*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMAttributeMap*>(aSupports);
  }

  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMAttributeMap)

private:
  Element *mContent; 

  


  AttrCache mAttributeCache;

  



  nsresult SetNamedItemInternal(nsIDOMNode *aNode,
                                nsIDOMNode **aReturn,
                                bool aWithNS);

  



  nsresult GetNamedItemNSInternal(const nsAString& aNamespaceURI,
                                  const nsAString& aLocalName,
                                  nsIDOMNode** aReturn,
                                  bool aRemove = false);

  nsDOMAttribute* GetAttribute(nsINodeInfo* aNodeInfo, bool aNsAware);

  


  nsresult RemoveAttribute(nsINodeInfo*     aNodeInfo,
                           nsIDOMNode**     aReturn);
};


#endif 
