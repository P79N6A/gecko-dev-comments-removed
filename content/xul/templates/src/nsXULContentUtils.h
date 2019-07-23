











































#ifndef nsXULContentUtils_h__
#define nsXULContentUtils_h__

#include "nsISupports.h"

class nsIAtom;
class nsIContent;
class nsIDocument;
class nsIDOMNodeList;
class nsIRDFNode;
class nsCString;
class nsString;
class nsIRDFResource;
class nsIRDFLiteral;
class nsIRDFService;
class nsINameSpaceManager;
class nsIDateTimeFormat;
class nsICollation;

class nsXULContentUtils
{
protected:
    static nsrefcnt gRefCnt;
    static nsIRDFService* gRDF;
    static nsIDateTimeFormat* gFormat;
    static nsICollation *gCollation;

    static PRBool gDisableXULCache;

    static int PR_CALLBACK
    DisableXULCacheChangedCallback(const char* aPrefName, void* aClosure);

public:
    static nsresult
    Init();

    static nsresult
    Finish();

    static nsresult
    FindChildByTag(nsIContent *aElement,
                   PRInt32 aNameSpaceID,
                   nsIAtom* aTag,
                   nsIContent **aResult);

    static nsresult
    FindChildByResource(nsIContent* aElement,
                        nsIRDFResource* aResource,
                        nsIContent** aResult);

    static nsresult
    GetElementResource(nsIContent* aElement, nsIRDFResource** aResult);

    static nsresult
    GetTextForNode(nsIRDFNode* aNode, nsAString& aResult);

    




    static nsresult
    MakeElementURI(nsIDocument* aDocument, const nsAString& aElementID, nsCString& aURI);

    static nsresult
    MakeElementResource(nsIDocument* aDocument, const nsAString& aElementID, nsIRDFResource** aResult);

    




    static nsresult
    MakeElementID(nsIDocument* aDocument, const nsACString& aURI, nsAString& aElementID);

    static nsresult
    GetResource(PRInt32 aNameSpaceID, nsIAtom* aAttribute, nsIRDFResource** aResult);

    static nsresult
    GetResource(PRInt32 aNameSpaceID, const nsAString& aAttribute, nsIRDFResource** aResult);

    static nsresult
    SetCommandUpdater(nsIDocument* aDocument, nsIContent* aElement);
    
    static nsIRDFService*
    RDFService()
    {
        return gRDF;
    }

    static nsICollation*
    GetCollation();

#define XUL_RESOURCE(ident, uri) static nsIRDFResource* ident
#define XUL_LITERAL(ident, val)  static nsIRDFLiteral*  ident
#include "nsXULResourceList.h"
#undef XUL_RESOURCE
#undef XUL_LITERAL
};

#endif 
