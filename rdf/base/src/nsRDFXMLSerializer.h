







































#ifndef nsRDFXMLSerializer_h__
#define nsRDFXMLSerializer_h__

#include "nsIRDFLiteral.h"
#include "nsIRDFXMLSerializer.h"
#include "nsIRDFXMLSource.h"
#include "nsNameSpaceMap.h"
#include "nsXPIDLString.h"

#include "nsDataHashtable.h"
#include "rdfITripleVisitor.h"

class nsString;
class nsIOutputStream;
class nsIRDFContainerUtils;






class nsRDFXMLSerializer : public nsIRDFXMLSerializer,
                           public nsIRDFXMLSource
{
public:
    static NS_IMETHODIMP
    Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIRDFXMLSERIALIZER
    NS_DECL_NSIRDFXMLSOURCE

protected:
    nsRDFXMLSerializer();
    virtual ~nsRDFXMLSerializer();

    
    nsresult
    RegisterQName(nsIRDFResource* aResource);
    nsresult
    GetQName(nsIRDFResource* aResource, nsCString& aQName);
    already_AddRefed<nsIAtom>
    EnsureNewPrefix();

    nsresult
    SerializeInlineAssertion(nsIOutputStream* aStream,
                             nsIRDFResource* aResource,
                             nsIRDFResource* aProperty,
                             nsIRDFLiteral* aValue);

    nsresult
    SerializeChildAssertion(nsIOutputStream* aStream,
                            nsIRDFResource* aResource,
                            nsIRDFResource* aProperty,
                            nsIRDFNode* aValue);

    nsresult
    SerializeProperty(nsIOutputStream* aStream,
                      nsIRDFResource* aResource,
                      nsIRDFResource* aProperty,
                      PRBool aInline,
                      PRInt32* aSkipped);

    PRBool
    IsContainerProperty(nsIRDFResource* aProperty);

    nsresult
    SerializeDescription(nsIOutputStream* aStream,
                         nsIRDFResource* aResource);

    nsresult
    SerializeMember(nsIOutputStream* aStream,
                    nsIRDFResource* aContainer,
                    nsIRDFNode* aMember);

    nsresult
    SerializeContainer(nsIOutputStream* aStream,
                       nsIRDFResource* aContainer);

    nsresult
    SerializePrologue(nsIOutputStream* aStream);

    nsresult
    SerializeEpilogue(nsIOutputStream* aStream);

    nsresult
    CollectNamespaces();

    PRBool
    IsA(nsIRDFDataSource* aDataSource, nsIRDFResource* aResource, nsIRDFResource* aType);

    nsCOMPtr<nsIRDFDataSource> mDataSource;
    nsNameSpaceMap mNameSpaces;
    nsXPIDLCString mBaseURLSpec;

    
    nsDataHashtable<nsISupportsHashKey, nsCString> mQNames;
    friend class QNameCollector;

    PRUint32 mPrefixID;

    static PRInt32 gRefCnt;
    static nsIRDFResource* kRDF_instanceOf;
    static nsIRDFResource* kRDF_type;
    static nsIRDFResource* kRDF_nextVal;
    static nsIRDFResource* kRDF_Bag;
    static nsIRDFResource* kRDF_Seq;
    static nsIRDFResource* kRDF_Alt;
    static nsIRDFContainerUtils* gRDFC;
};

#endif 
