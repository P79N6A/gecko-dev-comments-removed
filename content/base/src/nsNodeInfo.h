










#ifndef nsNodeInfo_h___
#define nsNodeInfo_h___

#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "plhash.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsGkAtoms.h"

class nsFixedSizeAllocator;

class nsNodeInfo : public nsINodeInfo
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsNodeInfo)

  
  virtual nsresult GetNamespaceURI(nsAString& aNameSpaceURI) const;
  virtual bool NamespaceEquals(const nsAString& aNamespaceURI) const;

  
  
public:
  


  static nsNodeInfo *Create(nsIAtom *aName, nsIAtom *aPrefix,
                            int32_t aNamespaceID, uint16_t aNodeType,
                            nsIAtom *aExtraName,
                            nsNodeInfoManager *aOwnerManager);
private:
  nsNodeInfo(); 
  nsNodeInfo(const nsNodeInfo& aOther); 
  nsNodeInfo(nsIAtom *aName, nsIAtom *aPrefix, int32_t aNamespaceID,
             uint16_t aNodeType, nsIAtom *aExtraName,
             nsNodeInfoManager *aOwnerManager);
protected:
  virtual ~nsNodeInfo();

public:
  


  static void ClearCache();

private:
  static nsFixedSizeAllocator* sNodeInfoPool;

  




  void LastRelease();
};

inline void
CheckValidNodeInfo(uint16_t aNodeType, nsIAtom *aName, int32_t aNamespaceID,
                   nsIAtom* aExtraName)
{
  NS_ABORT_IF_FALSE(aNodeType == nsIDOMNode::ELEMENT_NODE ||
                    aNodeType == nsIDOMNode::ATTRIBUTE_NODE ||
                    aNodeType == nsIDOMNode::TEXT_NODE ||
                    aNodeType == nsIDOMNode::CDATA_SECTION_NODE ||
                    aNodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE ||
                    aNodeType == nsIDOMNode::COMMENT_NODE ||
                    aNodeType == nsIDOMNode::DOCUMENT_NODE ||
                    aNodeType == nsIDOMNode::DOCUMENT_TYPE_NODE ||
                    aNodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE ||
                    aNodeType == PR_UINT16_MAX,
                    "Invalid nodeType");
  NS_ABORT_IF_FALSE((aNodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE ||
                     aNodeType == nsIDOMNode::DOCUMENT_TYPE_NODE) ==
                    !!aExtraName,
                    "Supply aExtraName for and only for PIs and doctypes");
  NS_ABORT_IF_FALSE(aNodeType == nsIDOMNode::ELEMENT_NODE ||
                    aNodeType == nsIDOMNode::ATTRIBUTE_NODE ||
                    aNodeType == PR_UINT16_MAX ||
                    aNamespaceID == kNameSpaceID_None,
                    "Only attributes and elements can be in a namespace");
  NS_ABORT_IF_FALSE(aName && aName != nsGkAtoms::_empty, "Invalid localName");
  NS_ABORT_IF_FALSE(((aNodeType == nsIDOMNode::TEXT_NODE) ==
                     (aName == nsGkAtoms::textTagName)) &&
                    ((aNodeType == nsIDOMNode::CDATA_SECTION_NODE) ==
                     (aName == nsGkAtoms::cdataTagName)) &&
                    ((aNodeType == nsIDOMNode::COMMENT_NODE) ==
                     (aName == nsGkAtoms::commentTagName)) &&
                    ((aNodeType == nsIDOMNode::DOCUMENT_NODE) ==
                     (aName == nsGkAtoms::documentNodeName)) &&
                    ((aNodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) ==
                     (aName == nsGkAtoms::documentFragmentNodeName)) &&
                    ((aNodeType == nsIDOMNode::DOCUMENT_TYPE_NODE) ==
                     (aName == nsGkAtoms::documentTypeNodeName)) &&
                    ((aNodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE) ==
                     (aName == nsGkAtoms::processingInstructionTagName)),
                    "Wrong localName for nodeType");
}

#endif 
