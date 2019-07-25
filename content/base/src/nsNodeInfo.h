










































#ifndef nsNodeInfo_h___
#define nsNodeInfo_h___

#include "nsNodeInfoManager.h"
#include "plhash.h"
#include "nsIAtom.h"
#include "nsINameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsDOMString.h"

class nsFixedSizeAllocator;

class nsNodeInfo
{
public:
  NS_INLINE_DECL_REFCOUNTING_WITH_DESTROY(nsNodeInfo, LastRelease())
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsNodeInfo)

  
  
  


  static nsNodeInfo *Create(nsIAtom *aName, nsIAtom *aPrefix,
                            PRInt32 aNamespaceID, PRUint16 aNodeType,
                            nsIAtom *aExtraName,
                            nsNodeInfoManager *aOwnerManager);

  





  void GetName(nsAString& aName) const
  {
    mInner.mName->ToString(aName);
  }

  






  nsIAtom* NameAtom() const
  {
    return mInner.mName;
  }

  






  const nsString& QualifiedName() const {
    return mQualifiedName;
  }

  


  const nsString& NodeName() const {
    return mNodeName;
  }

  


  const nsString& LocalName() const {
    return mLocalName;
  }

  





  void GetPrefix(nsAString& aPrefix) const
  {
    if (mInner.mPrefix) {
      mInner.mPrefix->ToString(aPrefix);
    } else {
      SetDOMStringToNull(aPrefix);
    }
  }

  





  nsIAtom* GetPrefixAtom() const
  {
    return mInner.mPrefix;
  }

  


  nsresult GetNamespaceURI(nsAString& aNameSpaceURI) const;

  



  PRInt32 NamespaceID() const
  {
    return mInner.mNamespaceID;
  }

  



  PRUint16 NodeType() const
  {
    return mInner.mNodeType;
  }

  


  nsIAtom* GetExtraName() const
  {
    return mInner.mExtraName;
  }

  





  nsIAtom* GetIDAttributeAtom() const
  {
    return mIDAttributeAtom;
  }

  void SetIDAttributeAtom(nsIAtom* aID)
  {
    mIDAttributeAtom = aID;
  }

  



  nsNodeInfoManager *NodeInfoManager() const
  {
    return mOwnerManager;
  }

  




  bool Equals(nsNodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName, mInner.mPrefix,
                                                  mInner.mNamespaceID);
  }

  bool NameAndNamespaceEquals(nsNodeInfo *aNodeInfo) const
  {
    return aNodeInfo == this || aNodeInfo->Equals(mInner.mName,
                                                  mInner.mNamespaceID);
  }

  bool Equals(nsIAtom *aNameAtom) const
  {
    return mInner.mName == aNameAtom;
  }

  bool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom) const
  {
    return (mInner.mName == aNameAtom) && (mInner.mPrefix == aPrefixAtom);
  }

  bool Equals(nsIAtom *aNameAtom, PRInt32 aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  bool Equals(nsIAtom *aNameAtom, nsIAtom *aPrefixAtom,
              PRInt32 aNamespaceID) const
  {
    return ((mInner.mName == aNameAtom) &&
            (mInner.mPrefix == aPrefixAtom) &&
            (mInner.mNamespaceID == aNamespaceID));
  }

  bool NamespaceEquals(PRInt32 aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID;
  }

  bool Equals(const nsAString& aName) const
  {
    return mInner.mName->Equals(aName);
  }

  bool Equals(const nsAString& aName, const nsAString& aPrefix) const
  {
    return mInner.mName->Equals(aName) &&
      (mInner.mPrefix ? mInner.mPrefix->Equals(aPrefix) : aPrefix.IsEmpty());
  }

  bool Equals(const nsAString& aName, PRInt32 aNamespaceID) const
  {
    return mInner.mNamespaceID == aNamespaceID &&
      mInner.mName->Equals(aName);
  }

  bool Equals(const nsAString& aName, const nsAString& aPrefix,
                PRInt32 aNamespaceID) const
  {
    return mInner.mName->Equals(aName) && mInner.mNamespaceID == aNamespaceID &&
      (mInner.mPrefix ? mInner.mPrefix->Equals(aPrefix) : aPrefix.IsEmpty());
  }

  bool NamespaceEquals(const nsAString& aNamespaceURI) const;

  bool QualifiedNameEquals(nsIAtom* aNameAtom) const
  {
    NS_PRECONDITION(aNameAtom, "Must have name atom");
    if (!GetPrefixAtom())
      return Equals(aNameAtom);

    return aNameAtom->Equals(mQualifiedName);
  }

  bool QualifiedNameEquals(const nsAString& aQualifiedName) const
  {
    return mQualifiedName == aQualifiedName;
  }

  


  nsIDocument* GetDocument() const
  {
    return mDocument;
  }

protected:
  













  class nsNodeInfoInner
  {
  public:
    nsNodeInfoInner()
      : mName(nsnull), mPrefix(nsnull), mNamespaceID(kNameSpaceID_Unknown),
        mNodeType(0), mNameString(nsnull), mExtraName(nsnull)
    {
    }
    nsNodeInfoInner(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                    PRUint16 aNodeType, nsIAtom* aExtraName)
      : mName(aName), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(nsnull), mExtraName(aExtraName)
    {
    }
    nsNodeInfoInner(const nsAString& aTmpName, nsIAtom *aPrefix,
                    PRInt32 aNamespaceID, PRUint16 aNodeType)
      : mName(nsnull), mPrefix(aPrefix), mNamespaceID(aNamespaceID),
        mNodeType(aNodeType), mNameString(&aTmpName), mExtraName(nsnull)
    {
    }

    nsIAtom*            mName;
    nsIAtom*            mPrefix;
    PRInt32             mNamespaceID;
    PRUint16            mNodeType; 
    const nsAString*    mNameString;
    nsIAtom*            mExtraName; 
  };

  
  friend class nsNodeInfoManager;

  nsIDocument* mDocument; 

  nsNodeInfoInner mInner;

  nsCOMPtr<nsIAtom> mIDAttributeAtom;
  nsNodeInfoManager* mOwnerManager; 

  




  
  nsString mQualifiedName;

  
  nsString mNodeName;

  
  
  nsString mLocalName;

private:
  nsNodeInfo(); 
  nsNodeInfo(const nsNodeInfo& aOther); 
  nsNodeInfo(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
             PRUint16 aNodeType, nsIAtom *aExtraName,
             nsNodeInfoManager *aOwnerManager);
protected:
  ~nsNodeInfo();

public:
  


  static void ClearCache();

private:
  static nsFixedSizeAllocator* sNodeInfoPool;

  




   void LastRelease();
};

#define CHECK_VALID_NODEINFO(_nodeType, _name, _namespaceID, _extraName)    \
NS_ABORT_IF_FALSE(_nodeType == nsIDOMNode::ELEMENT_NODE ||                  \
                  _nodeType == nsIDOMNode::ATTRIBUTE_NODE ||                \
                  _nodeType == nsIDOMNode::TEXT_NODE ||                     \
                  _nodeType == nsIDOMNode::CDATA_SECTION_NODE ||            \
                  _nodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE ||   \
                  _nodeType == nsIDOMNode::COMMENT_NODE ||                  \
                  _nodeType == nsIDOMNode::DOCUMENT_NODE ||                 \
                  _nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE ||            \
                  _nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE ||        \
                  _nodeType == PR_UINT16_MAX,                               \
                  "Invalid nodeType");                                      \
NS_ABORT_IF_FALSE((_nodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE ||  \
                   _nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE) ==          \
                  (_extraName != nsnull),                                   \
                  "Supply aExtraName for and only for PIs and doctypes");   \
NS_ABORT_IF_FALSE(_nodeType == nsIDOMNode::ELEMENT_NODE ||                  \
                  _nodeType == nsIDOMNode::ATTRIBUTE_NODE ||                \
                  _nodeType == PR_UINT16_MAX ||                             \
                  aNamespaceID == kNameSpaceID_None,                        \
                  "Only attributes and elements can be in a namespace");    \
NS_ABORT_IF_FALSE(_name && _name != nsGkAtoms::_empty, "Invalid localName");\
NS_ABORT_IF_FALSE(((_nodeType == nsIDOMNode::TEXT_NODE) ==                  \
                   (_name == nsGkAtoms::textTagName)) &&                    \
                  ((_nodeType == nsIDOMNode::CDATA_SECTION_NODE) ==         \
                   (_name == nsGkAtoms::cdataTagName)) &&                   \
                  ((_nodeType == nsIDOMNode::COMMENT_NODE) ==               \
                   (_name == nsGkAtoms::commentTagName)) &&                 \
                  ((_nodeType == nsIDOMNode::DOCUMENT_NODE) ==              \
                   (_name == nsGkAtoms::documentNodeName)) &&               \
                  ((_nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) ==     \
                   (_name == nsGkAtoms::documentFragmentNodeName)) &&       \
                  ((_nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE) ==         \
                   (_name == nsGkAtoms::documentTypeNodeName)) &&           \
                  ((_nodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE) ==\
                   (_name == nsGkAtoms::processingInstructionTagName)),     \
                  "Wrong localName for nodeType");

#endif 
