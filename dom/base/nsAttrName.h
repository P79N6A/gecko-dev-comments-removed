










#ifndef nsAttrName_h___
#define nsAttrName_h___

#include "mozilla/dom/NodeInfo.h"
#include "nsIAtom.h"
#include "nsDOMString.h"

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
    : mBits(reinterpret_cast<uintptr_t>(aAtom))
  {
    NS_ASSERTION(aAtom, "null atom-name in nsAttrName");
    NS_ADDREF(aAtom);
  }

  explicit nsAttrName(mozilla::dom::NodeInfo* aNodeInfo)
  {
    NS_ASSERTION(aNodeInfo, "null nodeinfo-name in nsAttrName");
    if (aNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
      mBits = reinterpret_cast<uintptr_t>(aNodeInfo->NameAtom());
      NS_ADDREF(aNodeInfo->NameAtom());
    }
    else {
      mBits = reinterpret_cast<uintptr_t>(aNodeInfo) |
              NS_ATTRNAME_NODEINFO_BIT;
      NS_ADDREF(aNodeInfo);
    }
  }

  ~nsAttrName()
  {
    ReleaseInternalName();
  }

  void SetTo(mozilla::dom::NodeInfo* aNodeInfo)
  {
    NS_ASSERTION(aNodeInfo, "null nodeinfo-name in nsAttrName");

    ReleaseInternalName();
    if (aNodeInfo->NamespaceEquals(kNameSpaceID_None)) {
      mBits = reinterpret_cast<uintptr_t>(aNodeInfo->NameAtom());
      NS_ADDREF(aNodeInfo->NameAtom());
    }
    else {
      mBits = reinterpret_cast<uintptr_t>(aNodeInfo) |
              NS_ATTRNAME_NODEINFO_BIT;
      NS_ADDREF(aNodeInfo);
    }
  }

  void SetTo(nsIAtom* aAtom)
  {
    NS_ASSERTION(aAtom, "null atom-name in nsAttrName");

    ReleaseInternalName();
    mBits = reinterpret_cast<uintptr_t>(aAtom);
    NS_ADDREF(aAtom);
  }

  bool IsAtom() const
  {
    return !(mBits & NS_ATTRNAME_NODEINFO_BIT);
  }

  mozilla::dom::NodeInfo* NodeInfo() const
  {
    NS_ASSERTION(!IsAtom(), "getting nodeinfo-value of atom-name");
    return reinterpret_cast<mozilla::dom::NodeInfo*>(mBits & ~NS_ATTRNAME_NODEINFO_BIT);
  }

  nsIAtom* Atom() const
  {
    NS_ASSERTION(IsAtom(), "getting atom-value of nodeinfo-name");
    return reinterpret_cast<nsIAtom*>(mBits);
  }

  bool Equals(const nsAttrName& aOther) const
  {
    return mBits == aOther.mBits;
  }

  
  bool Equals(nsIAtom* aAtom) const
  {
    return reinterpret_cast<uintptr_t>(aAtom) == mBits;
  }

  
  bool Equals(const nsAString& aLocalName) const
  {
    return IsAtom() && Atom()->Equals(aLocalName);
  }

  bool Equals(nsIAtom* aLocalName, int32_t aNamespaceID) const
  {
    if (aNamespaceID == kNameSpaceID_None) {
      return Equals(aLocalName);
    }
    return !IsAtom() && NodeInfo()->Equals(aLocalName, aNamespaceID);
  }

  bool Equals(mozilla::dom::NodeInfo* aNodeInfo) const
  {
    return Equals(aNodeInfo->NameAtom(), aNodeInfo->NamespaceID());
  }

  int32_t NamespaceID() const
  {
    return IsAtom() ? kNameSpaceID_None : NodeInfo()->NamespaceID();
  }

  int32_t NamespaceEquals(int32_t aNamespaceID) const
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
    return IsAtom() ? nullptr : NodeInfo()->GetPrefixAtom();
  }

  bool QualifiedNameEquals(const nsAString& aName) const
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

#ifdef MOZILLA_INTERNAL_API
  void GetPrefix(nsAString& aStr) const
  {
    if (IsAtom()) {
      SetDOMStringToNull(aStr);
    }
    else {
      NodeInfo()->GetPrefix(aStr);
    }
  }
#endif

  uint32_t HashValue() const
  {
    
    
    
    return mBits - 0;
  }

  bool IsSmaller(nsIAtom* aOther) const
  {
    return mBits < reinterpret_cast<uintptr_t>(aOther);
  }

private:

  void AddRefInternalName()
  {
    if (IsAtom()) {
      NS_ADDREF(Atom());
    } else {
      NS_ADDREF(NodeInfo());
    }
  }

  void ReleaseInternalName()
  {
    if (IsAtom()) {
      Atom()->Release();
    } else {
      NodeInfo()->Release();
    }
  }

  uintptr_t mBits;
};

#endif
