




































#ifndef relatedlinkshandler____h____
#define relatedlinkshandler____h____

#include "nsString.h"
#include "nsIRDFService.h"
#include "nsIRelatedLinksHandler.h"
#include "nsIRDFResource.h"
#include "nsCOMPtr.h"
#include "nsIRDFDataSource.h"




class RelatedLinksHandlerImpl : public nsIRelatedLinksHandler,
				public nsIRDFDataSource
{
private:
	char			*mRelatedLinksURL;
        static nsString         *mRLServerURL;

   
	static PRInt32		gRefCnt;
	static nsIRDFService    *gRDFService;
	static nsIRDFResource	*kNC_RelatedLinksRoot;
	static nsIRDFResource	*kNC_Child;
	static nsIRDFResource	*kRDF_type;
	static nsIRDFResource	*kNC_RelatedLinksTopic;

	nsCOMPtr<nsIRDFDataSource> mInner;
public:

				RelatedLinksHandlerImpl();
	virtual		~RelatedLinksHandlerImpl();
	nsresult	Init();


	NS_DECL_ISUPPORTS
	NS_DECL_NSIRELATEDLINKSHANDLER
	NS_DECL_NSIRDFDATASOURCE
};

#endif
