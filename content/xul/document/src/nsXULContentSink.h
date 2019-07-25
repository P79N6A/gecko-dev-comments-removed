








































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

    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIEXPATSINK

    
    NS_IMETHOD WillParse(void) { return NS_OK; }
    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
    NS_IMETHOD DidBuildModel(PRBool aTerminated);
    NS_IMETHOD WillInterrupt(void);
    NS_IMETHOD WillResume(void);
    NS_IMETHOD SetParser(nsIParser* aParser);
    virtual void FlushPendingNotifications(mozFlushType aType) { }
    NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
    virtual nsISupports *GetTarget();

    




    nsresult Init(nsIDocument* aDocument, nsXULPrototypeDocument* aPrototype);

protected:
    
    PRUnichar* mText;
    PRInt32 mTextLength;
    PRInt32 mTextSize;
    PRBool mConstrainSize;

    nsresult AddAttributes(const PRUnichar** aAttributes,
                           const PRUint32 aAttrLen,
                           nsXULPrototypeElement* aElement);

    nsresult OpenRoot(const PRUnichar** aAttributes,
                      const PRUint32 aAttrLen,
                      nsINodeInfo *aNodeInfo);

    nsresult OpenTag(const PRUnichar** aAttributes,
                     const PRUint32 aAttrLen,
                     const PRUint32 aLineNumber,
                     nsINodeInfo *aNodeInfo);

    
    
    
    
    
    
    nsresult OpenScript(const PRUnichar** aAttributes,
                        const PRUint32 aLineNumber);

    static PRBool IsDataInBuffer(PRUnichar* aBuffer, PRInt32 aLength);

    nsresult SetElementScriptType(nsXULPrototypeElement* element,
                                  const PRUnichar** aAttributes,
                                  const PRUint32 aAttrLen);

    
    nsresult FlushText(PRBool aCreateTextNode = PR_TRUE);
    nsresult AddText(const PRUnichar* aText, PRInt32 aLength);


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
        PRInt32 mDepth;

    public:
        ContextStack();
        ~ContextStack();

        PRInt32 Depth() { return mDepth; }

        nsresult Push(nsXULPrototypeNode* aNode, State aState);
        nsresult Pop(State* aState);

        nsresult GetTopNode(nsRefPtr<nsXULPrototypeNode>& aNode);
        nsresult GetTopChildren(nsPrototypeArray** aChildren);
        nsresult GetTopNodeScriptType(PRUint32 *aScriptType);

        void Clear();
    };

    friend class ContextStack;
    ContextStack mContextStack;

    nsWeakPtr              mDocument;             
    nsCOMPtr<nsIURI>       mDocumentURL;          

    nsRefPtr<nsXULPrototypeDocument> mPrototype;  

    
    nsIParser*             mParser;               
    nsCOMPtr<nsIScriptSecurityManager> mSecMan;
};

#endif
