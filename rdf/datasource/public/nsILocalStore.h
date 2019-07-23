




































#ifndef nsILocalStore_h__
#define nsILocalStore_h__

#include "rdf.h"
#include "nsISupports.h"


#define NS_ILOCALSTORE_IID \
{ 0xdf71c6f1, 0xec53, 0x11d2, { 0xbd, 0xca, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }


#define NS_LOCALSTORE_CID \
{ 0xdf71c6f0, 0xec53, 0x11d2, { 0xbd, 0xca, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

#define NS_LOCALSTORE_CONTRACTID NS_RDF_DATASOURCE_CONTRACTID_PREFIX "local-store"

class nsILocalStore : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILOCALSTORE_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILocalStore, NS_ILOCALSTORE_IID)

extern NS_IMETHODIMP
NS_NewLocalStore(nsISupports* aOuter, REFNSIID aIID, void** aResult);


#endif
