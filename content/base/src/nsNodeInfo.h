










































#ifndef nsNodeInfo_h___
#define nsNodeInfo_h___

#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "plhash.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

class nsFixedSizeAllocator;

class nsNodeInfo : public nsINodeInfo
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsNodeInfo)

  
  virtual nsresult GetNamespaceURI(nsAString& aNameSpaceURI) const;
  virtual PRBool NamespaceEquals(const nsAString& aNamespaceURI) const;

  
  
public:
  


  static nsNodeInfo *Create(nsIAtom *aName, nsIAtom *aPrefix,
                            PRInt32 aNamespaceID, PRUint16 aNodeType,
                            nsIAtom *aExtraName,
                            nsNodeInfoManager *aOwnerManager);
private:
  nsNodeInfo(); 
  nsNodeInfo(const nsNodeInfo& aOther); 
  nsNodeInfo(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
             PRUint16 aNodeType, nsIAtom *aExtraName,
             nsNodeInfoManager *aOwnerManager);
protected:
  virtual ~nsNodeInfo();

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
