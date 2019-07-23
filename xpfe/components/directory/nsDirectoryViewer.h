






































#ifndef nsdirectoryviewer__h____
#define nsdirectoryviewer__h____

#include "nsNetUtil.h"
#include "nsIStreamListener.h"
#include "nsIContentViewer.h"
#ifdef MOZ_RDF
#include "nsIHTTPIndex.h"
#include "nsIRDFService.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFLiteral.h"
#endif
#include "nsIDocumentLoaderFactory.h"
#include "nsITimer.h"
#include "nsISupportsArray.h"
#include "nsXPIDLString.h"
#include "nsIDirIndexListener.h"
#include "nsIFTPChannel.h"

class nsDirectoryViewerFactory : public nsIDocumentLoaderFactory
{
public:
    nsDirectoryViewerFactory();
    virtual ~nsDirectoryViewerFactory();

    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOCUMENTLOADERFACTORY
};

#ifdef MOZ_RDF
class nsHTTPIndex : public nsIHTTPIndex,
                    public nsIRDFDataSource,
                    public nsIStreamListener,
                    public nsIDirIndexListener,
                    public nsIInterfaceRequestor,
                    public nsIFTPEventSink
{
private:

    
    

    nsCOMPtr<nsIRDFResource>     kNC_Child;
    nsCOMPtr<nsIRDFResource>     kNC_Comment;
    nsCOMPtr<nsIRDFResource>     kNC_Loading;
    nsCOMPtr<nsIRDFResource>     kNC_URL;
    nsCOMPtr<nsIRDFResource>     kNC_Description;
    nsCOMPtr<nsIRDFResource>     kNC_ContentLength;
    nsCOMPtr<nsIRDFResource>     kNC_LastModified;
    nsCOMPtr<nsIRDFResource>     kNC_ContentType;
    nsCOMPtr<nsIRDFResource>     kNC_FileType;
    nsCOMPtr<nsIRDFResource>     kNC_IsContainer;
    nsCOMPtr<nsIRDFLiteral>      kTrueLiteral;
    nsCOMPtr<nsIRDFLiteral>      kFalseLiteral;

    nsCOMPtr<nsIRDFService>      mDirRDF;

protected:
    
    
    
    
    

    nsCOMPtr<nsIRDFDataSource>   mInner;
    nsCOMPtr<nsISupportsArray>   mConnectionList;
    nsCOMPtr<nsISupportsArray>   mNodeList;
    nsCOMPtr<nsITimer>           mTimer;
    nsCOMPtr<nsIDirIndexParser>  mParser;
    nsCString mBaseURL;
    nsCString                    mEncoding;
    PRBool                       mBindToGlobalObject;
    nsIInterfaceRequestor*       mRequestor; 
    nsCOMPtr<nsIRDFResource>     mDirectory;

    nsHTTPIndex(nsIInterfaceRequestor* aRequestor);
    nsresult CommonInit(void);
    nsresult Init(nsIURI* aBaseURL);
    void        GetDestination(nsIRDFResource* r, nsXPIDLCString& dest);
    PRBool      isWellknownContainerURI(nsIRDFResource *r);
    nsresult    AddElement(nsIRDFResource *parent, nsIRDFResource *prop,
                           nsIRDFNode *child);

    static void FireTimer(nsITimer* aTimer, void* aClosure);

public:
    nsHTTPIndex();
    virtual ~nsHTTPIndex();
    nsresult Init(void);

    static nsresult Create(nsIURI* aBaseURI, nsIInterfaceRequestor* aContainer,
                           nsIHTTPIndex** aResult);

    
    NS_DECL_NSIHTTPINDEX

    
    NS_DECL_NSIRDFDATASOURCE

    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    
    NS_DECL_NSIDIRINDEXLISTENER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIFTPEVENTSINK

    
    NS_DECL_ISUPPORTS
};
#endif


#define NS_DIRECTORYVIEWERFACTORY_CID \
{ 0x82776710, 0x5690, 0x11d3, { 0xbe, 0x36, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }


#endif
