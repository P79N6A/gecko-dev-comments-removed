









































#ifndef nsDOMAttributeMap_h___
#define nsDOMAttributeMap_h___

#include "nsIDOMNamedNodeMap.h"
#include "nsString.h"
#include "nsInterfaceHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "prbit.h"
#include "nsIDOMNode.h"

class nsIAtom;
class nsIContent;
class nsDOMAttribute;
class nsINodeInfo;
class nsIDocument;




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
  PRBool KeyEquals(KeyTypePointer aKey) const
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
  nsDOMAttributeMap(nsIContent* aContent);
  virtual ~nsDOMAttributeMap();

  


  PRBool Init();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMNAMEDNODEMAP

  void DropReference();

  nsIContent* GetContent()
  {
    return mContent;
  }

  



  nsresult SetOwnerDocument(nsIDocument* aDocument);

  



  void DropAttribute(PRInt32 aNamespaceID, nsIAtom* aLocalName);

  






  PRUint32 Count() const;

  typedef nsInterfaceHashtable<nsAttrHashKey, nsIDOMNode> AttrCache;

  





  PRUint32 Enumerate(AttrCache::EnumReadFunction aFunc, void *aUserArg) const;

  nsIDOMNode* GetItemAt(PRUint32 aIndex, nsresult *rv);

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
  nsIContent* mContent; 

  


  AttrCache mAttributeCache;

  



  nsresult SetNamedItemInternal(nsIDOMNode *aNode,
                                nsIDOMNode **aReturn,
                                PRBool aWithNS);

  



  nsresult GetNamedItemNSInternal(const nsAString& aNamespaceURI,
                                  const nsAString& aLocalName,
                                  nsIDOMNode** aReturn,
                                  PRBool aRemove = PR_FALSE);

  



  nsresult GetAttribute(nsINodeInfo*     aNodeInfo,
                        nsIDOMNode**     aReturn)
  {
    *aReturn = GetAttribute(aNodeInfo);
    if (!*aReturn) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(*aReturn);

    return NS_OK;
  }

  nsIDOMNode* GetAttribute(nsINodeInfo*     aNodeInfo);

  


  nsresult RemoveAttribute(nsINodeInfo*     aNodeInfo,
                           nsIDOMNode**     aReturn);
};


#endif 
