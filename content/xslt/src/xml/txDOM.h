















































#ifndef MITRE_DOM
#define MITRE_DOM

#ifdef __BORLANDC__
#include <stdlib.h>
#endif

#include "txList.h"
#include "nsIAtom.h"
#include "nsDoubleHashtable.h"
#include "nsString.h"
#include "txCore.h"
#include "nsAutoPtr.h"

#define kTxNsNodeIndexOffset 0x00000000;
#define kTxAttrIndexOffset 0x40000000;
#define kTxChildIndexOffset 0x80000000;

class NamedNodeMap;
class Document;
class Element;
class Attr;
class ProcessingInstruction;

#define kNameSpaceID_Unknown -1
#define kNameSpaceID_None     0

#define kNameSpaceID_XMLNS    1 
#define kNameSpaceID_XML      2

#define kNameSpaceID_XSLT     3





class Node : public TxObject
{
  public:
    
    
    enum NodeType {
        ELEMENT_NODE = 1,
        ATTRIBUTE_NODE,
        TEXT_NODE,
        CDATA_SECTION_NODE,
        ENTITY_REFERENCE_NODE,
        ENTITY_NODE,
        PROCESSING_INSTRUCTION_NODE,
        COMMENT_NODE,
        DOCUMENT_NODE,
        DOCUMENT_TYPE_NODE,
        DOCUMENT_FRAGMENT_NODE,
        NOTATION_NODE
    };

    
    virtual nsresult getNodeName(nsAString& aName) const = 0;
    virtual nsresult getNodeValue(nsAString& aValue) = 0;
    virtual unsigned short getNodeType() const = 0;
    virtual Node* getParentNode() const = 0;
    virtual Node* getFirstChild() const = 0;
    virtual Node* getLastChild() const = 0;
    virtual Node* getPreviousSibling() const = 0;
    virtual Node* getNextSibling() const = 0;
    virtual Document* getOwnerDocument() const = 0;

    
    virtual Node* appendChild(Node* newChild) = 0;

    virtual MBool hasChildNodes() const = 0;
    
    
    virtual nsresult getBaseURI(nsAString& aURI) = 0;

    
    virtual nsresult getNamespaceURI(nsAString& aNSURI) = 0;

    
    virtual MBool getLocalName(nsIAtom** aLocalName) = 0;
    virtual PRInt32 getNamespaceID() = 0;
    virtual Node* getXPathParent() = 0;
    virtual PRInt32 compareDocumentPosition(Node* aOther) = 0;
};







class NodeDefinition : public Node
{
  public:
    virtual ~NodeDefinition();      

    
    virtual nsresult getNodeName(nsAString& aName) const;
    nsresult getNodeValue(nsAString& aValue);
    unsigned short getNodeType() const;
    Node* getParentNode() const;
    Node* getFirstChild() const;
    Node* getLastChild() const;
    Node* getPreviousSibling() const;
    Node* getNextSibling() const;
    Document* getOwnerDocument() const;

    
    virtual Node* appendChild(Node* newChild);

    MBool hasChildNodes() const;
    
    
    virtual nsresult getBaseURI(nsAString& aURI);

    
    nsresult getNamespaceURI(nsAString& aNSURI);

    
    virtual MBool getLocalName(nsIAtom** aLocalName);
    virtual PRInt32 getNamespaceID();
    virtual Node* getXPathParent();
    virtual PRInt32 compareDocumentPosition(Node* aOther);

    
    void appendData(const PRUnichar* aData, int aLength)
    {
      nodeValue.Append(aData, aLength);
    };

  protected:
    friend class Document;
    friend class txXPathTreeWalker;
    friend class txXPathNodeUtils;
    NodeDefinition(NodeType type, nsIAtom *aLocalName,
                   const nsAString& value, Document* owner);

    
    
    
    nsCOMPtr<nsIAtom> mLocalName;
    nsString nodeValue;

    NodeDefinition* implAppendChild(NodeDefinition* newChild);
    NodeDefinition* implRemoveChild(NodeDefinition* oldChild);

  private:
    
    NodeType nodeType;

    
    NodeDefinition* parentNode;
    NodeDefinition* previousSibling;
    NodeDefinition* nextSibling;

    
    Document* ownerDocument;

    PRUint32 length;

    
    NodeDefinition* firstChild;
    NodeDefinition* lastChild;

    
    struct OrderInfo {
        ~OrderInfo();
        PRUint32* mOrder;
        PRInt32 mSize;
        Node* mRoot;
    };

    
    OrderInfo* mOrderInfo;

    
    OrderInfo* getOrderInfo();
};














class txIDEntry : public PLDHashStringEntry
{
public:
    txIDEntry(const void* aKey) : PLDHashStringEntry(aKey), mElement(nsnull)
    {
    }
    Element* mElement;
};
DECL_DHASH_WRAPPER(txIDMap, txIDEntry, nsAString&)

class Document : public NodeDefinition
{
  public:
    Document();

    Element* getDocumentElement();

    
    Node* createComment(const nsAString& aData);
    ProcessingInstruction* createProcessingInstruction(nsIAtom *aTarget,
                                                       const nsAString& aData);
    Node* createTextNode(const nsAString& theData);

    Element* createElementNS(nsIAtom *aPrefix, nsIAtom *aLocalName,
                             PRInt32 aNamespaceID);
    Element* getElementById(const nsAString& aID);

    
    Node* appendChild(Node* newChild);

    
    nsresult getBaseURI(nsAString& aURI);

  private:
    PRBool setElementID(const nsAString& aID, Element* aElement);

    Element* documentElement;

    
    
    friend class txXMLParser;
    txIDMap mIDMap;
    nsString documentBaseURI;
};




class Element : public NodeDefinition
{
  public:
    NamedNodeMap* getAttributes();

    nsresult appendAttributeNS(nsIAtom *aPrefix, nsIAtom *aLocalName,
                               PRInt32 aNamespaceID, const nsAString& aValue);

    
    Node* appendChild(Node* newChild);

    
    nsresult getNodeName(nsAString& aName) const;
    MBool getLocalName(nsIAtom** aLocalName);
    PRInt32 getNamespaceID();
    MBool getAttr(nsIAtom* aLocalName, PRInt32 aNSID, nsAString& aValue);
    MBool hasAttr(nsIAtom* aLocalName, PRInt32 aNSID);

    
    PRBool getIDValue(nsAString& aValue);

    Attr *getFirstAttribute()
    {
      return mFirstAttribute;
    }

  private:
    friend class Document;
    void setIDValue(const nsAString& aValue);

    Element(nsIAtom *aPrefix, nsIAtom *aLocalName, PRInt32 aNamespaceID,
            Document* aOwner);

    nsAutoPtr<Attr> mFirstAttribute;
    nsString mIDValue;
    nsCOMPtr<nsIAtom> mPrefix;
    PRInt32 mNamespaceID;
};






class Attr : public NodeDefinition
{
  public:
    Node* appendChild(Node* newChild);

    
    nsresult getNodeName(nsAString& aName) const;
    MBool getLocalName(nsIAtom** aLocalName);
    PRInt32 getNamespaceID();
    Node* getXPathParent();
    PRBool equals(nsIAtom *aLocalName, PRInt32 aNamespaceID)
    {
      return mLocalName == aLocalName && aNamespaceID == mNamespaceID;
    }
    Attr *getNextAttribute()
    {
      return mNextAttribute;
    }

  private:
    friend class Element;

    Attr(nsIAtom *aPrefix, nsIAtom *aLocalName, PRInt32 aNamespaceID,
         Element *aOwnerElement, const nsAString &aValue);

    nsCOMPtr<nsIAtom> mPrefix;
    PRInt32 mNamespaceID;
    Element *mOwnerElement;
    nsAutoPtr<Attr> mNextAttribute;
};









class ProcessingInstruction : public NodeDefinition
{
  public:
    
    MBool getLocalName(nsIAtom** aLocalName);

  private:
    friend class Document;
    ProcessingInstruction(nsIAtom *theTarget, const nsAString& theData,
                          Document* owner);
};

class txStandaloneNamespaceManager
{
public:
    static PRInt32 getNamespaceID(const nsAString& aURI)
    {
        if (!mNamespaces && !init())
            return kNameSpaceID_Unknown;

        PRInt32 id = mNamespaces->IndexOf(aURI);
        if (id != -1) {
            return id + 1;
        }

        if (!mNamespaces->AppendString(aURI)) {
            NS_ERROR("Out of memory, namespaces are getting lost");
            return kNameSpaceID_Unknown;
        }

        return mNamespaces->Count();
    }

    static nsresult getNamespaceURI(const PRInt32 aID, nsAString& aNSURI)
    {
        
        aNSURI.Truncate();
        if (aID <= 0 || (!mNamespaces && !init()) ||
            aID > mNamespaces->Count()) {
            return NS_OK;
        }

        aNSURI = *mNamespaces->StringAt(aID - 1);
        return NS_OK;
    }

    static MBool init()
    {
        NS_ASSERTION(!mNamespaces,
                     "called without matching shutdown()");
        if (mNamespaces)
            return MB_TRUE;
        mNamespaces = new nsStringArray();
        if (!mNamespaces)
            return MB_FALSE;
        





        if (!mNamespaces->AppendString(NS_LITERAL_STRING("http://www.w3.org/2000/xmlns/")) ||
            !mNamespaces->AppendString(NS_LITERAL_STRING("http://www.w3.org/XML/1998/namespace")) ||
            !mNamespaces->AppendString(NS_LITERAL_STRING("http://www.w3.org/1999/XSL/Transform"))) {
            delete mNamespaces;
            mNamespaces = 0;
            return MB_FALSE;
        }

        return MB_TRUE;
    }

    static void shutdown()
    {
        NS_ASSERTION(mNamespaces, "called without matching init()");
        if (!mNamespaces)
            return;
        delete mNamespaces;
        mNamespaces = nsnull;
    }

private:
    static nsStringArray* mNamespaces;
};

#define TX_IMPL_DOM_STATICS \
    nsStringArray* txStandaloneNamespaceManager::mNamespaces = 0

#endif
