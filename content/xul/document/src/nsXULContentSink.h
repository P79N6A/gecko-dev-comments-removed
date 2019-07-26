




#ifndef nsXULContentSink_h__
#define nsXULContentSink_h__

#include "nsIExpatSink.h"
#include "nsIXMLContentSink.h"
#include "nsAutoPtr.h"
#include "nsNodeInfoManager.h"
#include "nsWeakPtr.h"
#include "nsXULElement.h"
#include "nsIDTD.h"

class nsIDocument;
class nsIScriptSecurityManager;
class nsAttrName;
class nsXULPrototypeDocument;
class nsXULPrototypeElement;
class nsXULPrototypeNode;

class XULContentSinkImpl : public nsIXMLContentSink,
                           public nsIExpatSink
{
public:
    XULContentSinkImpl();
    virtual ~XULContentSinkImpl();

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIEXPATSINK

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(XULContentSinkImpl, nsIXMLContentSink)

    
    NS_IMETHOD WillParse(void) { return NS_OK; }
    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
    NS_IMETHOD DidBuildModel(bool aTerminated);
    NS_IMETHOD WillInterrupt(void);
    NS_IMETHOD WillResume(void);
    NS_IMETHOD SetParser(nsParserBase* aParser);
    virtual void FlushPendingNotifications(mozFlushType aType) { }
    NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
    virtual nsISupports *GetTarget();

    




    nsresult Init(nsIDocument* aDocument, nsXULPrototypeDocument* aPrototype);

protected:
    
    PRUnichar* mText;
    int32_t mTextLength;
    int32_t mTextSize;
    bool mConstrainSize;

    nsresult AddAttributes(const PRUnichar** aAttributes,
                           const uint32_t aAttrLen,
                           nsXULPrototypeElement* aElement);

    nsresult OpenRoot(const PRUnichar** aAttributes,
                      const uint32_t aAttrLen,
                      nsINodeInfo *aNodeInfo);

    nsresult OpenTag(const PRUnichar** aAttributes,
                     const uint32_t aAttrLen,
                     const uint32_t aLineNumber,
                     nsINodeInfo *aNodeInfo);

    
    
    
    
    
    
    nsresult OpenScript(const PRUnichar** aAttributes,
                        const uint32_t aLineNumber);

    static bool IsDataInBuffer(PRUnichar* aBuffer, int32_t aLength);

    
    nsresult FlushText(bool aCreateTextNode = true);
    nsresult AddText(const PRUnichar* aText, int32_t aLength);


    nsRefPtr<nsNodeInfoManager> mNodeInfoManager;

    nsresult NormalizeAttributeString(const PRUnichar *aExpatName,
                                      nsAttrName &aName);
    nsresult CreateElement(nsINodeInfo *aNodeInfo,
                           nsXULPrototypeElement** aResult);


    public:
    enum State { eInProlog, eInDocumentElement, eInScript, eInEpilog };
    protected:

    State mState;

    
    class ContextStack {
    protected:
        struct Entry {
            nsRefPtr<nsXULPrototypeNode> mNode;
            
            nsPrototypeArray    mChildren;
            State               mState;
            Entry*              mNext;
            Entry() : mChildren(8) {}
        };

        Entry* mTop;
        int32_t mDepth;

    public:
        ContextStack();
        ~ContextStack();

        int32_t Depth() { return mDepth; }

        nsresult Push(nsXULPrototypeNode* aNode, State aState);
        nsresult Pop(State* aState);

        nsresult GetTopNode(nsRefPtr<nsXULPrototypeNode>& aNode);
        nsresult GetTopChildren(nsPrototypeArray** aChildren);

        void Clear();

        void Traverse(nsCycleCollectionTraversalCallback& aCallback);
    };

    friend class ContextStack;
    ContextStack mContextStack;

    nsWeakPtr              mDocument;             
    nsCOMPtr<nsIURI>       mDocumentURL;          

    nsRefPtr<nsXULPrototypeDocument> mPrototype;  

    
    nsParserBase*         mParser;               
    nsCOMPtr<nsIScriptSecurityManager> mSecMan;
};

#endif
