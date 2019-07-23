








































#ifndef nsXULContentSink_h__
#define nsXULContentSink_h__

#include "nsIExpatSink.h"
#include "nsIXMLContentSink.h"
#include "nsAutoPtr.h"
#include "nsNodeInfoManager.h"
#include "nsVoidArray.h"
#include "nsWeakPtr.h"

class nsICSSLoader;
class nsICSSParser;
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

    
    NS_IMETHOD WillTokenize(void) { return NS_OK; }
    NS_IMETHOD WillBuildModel(void);
    NS_IMETHOD DidBuildModel(void);
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
            nsXULPrototypeNode* mNode;
            
            nsAutoVoidArray     mChildren;
            State               mState;
            Entry*              mNext;
        };

        Entry* mTop;
        PRInt32 mDepth;

    public:
        ContextStack();
        ~ContextStack();

        PRInt32 Depth() { return mDepth; }

        nsresult Push(nsXULPrototypeNode* aNode, State aState);
        nsresult Pop(State* aState);

        nsresult GetTopNode(nsXULPrototypeNode** aNode);
        nsresult GetTopChildren(nsVoidArray** aChildren);
        nsresult GetTopNodeScriptType(PRUint32 *aScriptType);
    };

    friend class ContextStack;
    ContextStack mContextStack;

    nsWeakPtr              mDocument;             
    nsCOMPtr<nsIURI>       mDocumentURL;          

    nsRefPtr<nsXULPrototypeDocument> mPrototype;  

    
    nsIParser*             mParser;               
    nsCOMPtr<nsICSSLoader> mCSSLoader;            
    nsCOMPtr<nsICSSParser> mCSSParser;            
    nsCOMPtr<nsIScriptSecurityManager> mSecMan;
};

#endif
