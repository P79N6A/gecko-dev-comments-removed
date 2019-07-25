











































#ifndef nsAttrName_h___
#define nsAttrName_h___

#include "nsINodeInfo.h"
#include "nsIAtom.h"
#include "nsDOMString.h"

typedef PRUptrdiff PtrBits;

#define NS_ATTRNAME_NODEINFO_BIT 1
class nsAttrName
{
public:
  nsAttrName(const nsAttrName& aOther)
    : mBits(aOther.mBits)
  {
    AddRefInternalName();
  }

  explicit nsAttrName(nsIAtom* aAtom)
    : mBits(reinterpret_cast<PtrBits>(aAtom))
  {
    NS_ASSERTION(aAtom, "null atom-name in nsAttrName");
    NS_ADDREF(aAtom);
  }

  explicit nsAttrName(nsINodeInfo* aNodeInfo)
  {
    NS_ASSERTION(aNodeInfo, "null nodeinfo-name in nsAttrName");
    if (aNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
      mBits = reinterpret_cast<PtrBits>(aNodeInfo->NameAtom());
      NS_ADDREF(aNodeInfo->NameAtom());
    }
    else {
      mBits = reinterpret_cast<PtrBits>(aNodeInfo) |
              NS_ATTRNAME_NODEINFO_BIT;
      NS_ADDREF(aNodeInfo);
    }
  }

  ~nsAttrName()
  {
    ReleaseInternalName();
  }

  void SetTo(nsINodeInfo* aNodeInfo)
  {
    NS_ASSERTION(aNodeInfo, "null nodeinfo-name in nsAttrName");

    ReleaseInternalName();
    if (aNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
      mBits = reinterpret_cast<PtrBits>(aNodeInfo->NameAtom());
      NS_ADDREF(aNodeInfo->NameAtom());
    }
    else {
      mBits = reinterpret_cast<PtrBits>(aNodeInfo) |
              NS_ATTRNAME_NODEINFO_BIT;
      NS_ADDREF(aNodeInfo);
    }
  }

  void SetTo(nsIAtom* aAtom)
  {
    NS_ASSERTION(aAtom, "null atom-name in nsAttrName");

    ReleaseInternalName();
    mBits = reinterpret_cast<PtrBits>(aAtom);
    NS_ADDREF(aAtom);
  }

  PRBool IsAtom() const
  {
    return !(mBits & NS_ATTRNAME_NODEINFO_BIT);
  }

  nsINodeInfo* NodeInfo() const
  {
    NS_ASSERTION(!IsAtom(), "getting nodeinfo-value of atom-name");
    return reinterpret_cast<nsINodeInfo*>(mBits & ~NS_ATTRNAME_NODEINFO_BIT);
  }

  nsIAtom* Atom() const
  {
    NS_ASSERTION(IsAtom(), "getting atom-value of nodeinfo-name");
    return reinterpret_cast<nsIAtom*>(mBits);
  }

  PRBool Equals(const nsAttrName& aOther) const
  {
    return mBits == aOther.mBits;
  }

  
  PRBool Equals(nsIAtom* aAtom) const
  {
    return reinterpret_cast<PtrBits>(aAtom) == mBits;
  }

  PRBool Equals(nsIAtom* aLocalName, PRInt32 aNamespaceID) const
  {
    if (aNamespaceID == kNameSpaceID_None) {
      return Equals(aLocalName);
    }
    return !IsAtom() && NodeInfo()->Equals(aLocalName, aNamespaceID);
  }

  PRBool Equals(nsINodeInfo* aNodeInfo) const
  {
    return Equals(aNodeInfo->NameAtom(), aNodeInfo->NamespaceID());
  }

  PRInt32 NamespaceID() const
  {
    return IsAtom() ? kNameSpaceID_None : NodeInfo()->NamespaceID();
  }

  PRInt32 NamespaceEquals(PRInt32 aNamespaceID) const
  {
    return aNamespaceID == kNameSpaceID_None ?
           IsAtom() :
           (!IsAtom() && NodeInfo()->NamespaceEquals(aNamespaceID));
  }

  nsIAtom* LocalName() const
  {
    return IsAtom() ? Atom() : NodeInfo()->NameAtom();
  }

  nsIAtom* GetPrefix() const
  {
    return IsAtom() ? nsnull : NodeInfo()->GetPrefixAtom();
  }

  PRBool QualifiedNameEquals(const nsAString& aName) const
  {
    return IsAtom() ? Atom()->Equals(aName) :
                      NodeInfo()->QualifiedNameEquals(aName);
  }

  void GetQualifiedName(nsAString& aStr) const
  {
    if (IsAtom()) {
      Atom()->ToString(aStr);
    }
    else {
      aStr = NodeInfo()->QualifiedName();
    }
  }

  void GetPrefix(nsAString& aStr) const
  {
    if (IsAtom()) {
      SetDOMStringToNull(aStr);
    }
    else {
      NodeInfo()->GetPrefix(aStr);
    }
  }

  PRUint32 HashValue() const
  {
    
    
    
    return mBits - 0;
  }

  PRBool IsSmaller(nsIAtom* aOther) const
  {
    return mBits < reinterpret_cast<PtrBits>(aOther);
  }

private:

  void AddRefInternalName()
  {
    
    
    nsISupports* name = reinterpret_cast<nsISupports *>
                                        (mBits & ~NS_ATTRNAME_NODEINFO_BIT);

    NS_ADDREF(name);
  }

  void ReleaseInternalName()
  {
    
    
    nsISupports* name = reinterpret_cast<nsISupports *>
                                        (mBits & ~NS_ATTRNAME_NODEINFO_BIT);

    NS_RELEASE(name);
  }

  PtrBits mBits;
};

#endif
