




































#ifndef localsearchdb___h_____
#define localsearchdb___h_____

#include "nsISupportsArray.h"
#include "nsCOMPtr.h"
#include "nsIRDFDataSource.h"
#include "nsString.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsISearchService.h"

typedef	struct	_findTokenStruct
{
	const char		*token;
	nsString	    value;
} findTokenStruct, *findTokenPtr;


class LocalSearchDataSource : public nsIRDFDataSource
{
private:
	static PRInt32		gRefCnt;

    
	static nsIRDFResource	*kNC_Child;
	static nsIRDFResource	*kNC_Name;
	static nsIRDFResource	*kNC_URL;
	static nsIRDFResource	*kNC_FindObject;
	static nsIRDFResource	*kNC_pulse;
	static nsIRDFResource	*kRDF_InstanceOf;
	static nsIRDFResource	*kRDF_type;

protected:

	NS_METHOD	getFindResults(nsIRDFResource *source, nsISimpleEnumerator** aResult);
	NS_METHOD	getFindName(nsIRDFResource *source, nsIRDFLiteral** aResult);
	NS_METHOD	parseResourceIntoFindTokens(nsIRDFResource *u, findTokenPtr tokens);

    
	PRBool doMatch(nsIRDFLiteral  *literal,
                   const nsAString& matchMethod,
                   const nsString& matchText);
    PRBool matchNode(nsIRDFNode *aNode,
                     const nsAString& matchMethod,
                     const nsString& matchText);
    
	PRBool doDateMatch(nsIRDFDate *literal,
                       const nsAString& matchMethod,
                       const nsAString& matchText);
	PRBool doIntMatch (nsIRDFInt  *literal,
                       const nsAString& matchMethod,
                       const nsString& matchText);

    PRBool dateMatches(nsIRDFDate *literal,
                       const nsAString& method,
                       const PRInt64& matchDate);
    
    NS_METHOD   parseDate(const nsAString& aDate, PRInt64* aResult);
    
	NS_METHOD	parseFindURL(nsIRDFResource *u, nsISupportsArray *array);

public:
	LocalSearchDataSource(void);
	virtual		~LocalSearchDataSource(void);
	nsresult	Init();

	NS_DECL_ISUPPORTS
	NS_DECL_NSILOCALSEARCHSERVICE
    NS_DECL_NSIRDFDATASOURCE
};

#endif
