














































#ifndef nsBaseDataSources_h__
#define nsBaseDataSources_h__

#include "nsError.h"
class nsIRDFDataSource;


nsresult
NS_NewRDFInMemoryDataSource(nsISupports* aOuter, const nsIID& aIID, void** aResult);


extern nsresult
NS_NewRDFXMLDataSource(nsIRDFDataSource** aResult);

#endif 


