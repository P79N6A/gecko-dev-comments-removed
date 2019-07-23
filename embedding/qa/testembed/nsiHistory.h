










































#include "nsIHistoryEntry.h"




class CNsIHistory
{

public:
	CNsIHistory(nsIWebNavigation *mWebNav);


public:
	
	
	nsCOMPtr<nsIWebNavigation> qaWebNav;
	nsCOMPtr<nsIURI> theURI;
	

public:


public:
	virtual ~CNsIHistory();

public:

	
	void GetCountTest(nsISHistory *, PRInt32 *, PRInt16);
	void GetIndexTest(nsISHistory *, PRInt32 *, PRInt16);
	void GetMaxLengthTest(nsISHistory *, PRInt32 *, PRInt16);
	void SetMaxLengthTest(nsISHistory *, PRInt32, PRInt16);
	void GetEntryAtIndexTest(nsISHistory *, nsIHistoryEntry *, PRInt32 theIndex, PRInt16);
	void GetURIHistTest(nsIHistoryEntry *, PRInt16);
	void GetTitleHistTest(nsIHistoryEntry *, PRInt16);
	void GetIsSubFrameTest(nsIHistoryEntry *, PRInt16);
	void GetSHEnumTest(nsISHistory*, nsISimpleEnumerator *, PRInt16);
	void SimpleEnumTest(nsISimpleEnumerator *, PRInt16);
	void PurgeHistoryTest(nsISHistory *, PRInt32, PRInt16);
	void RunAllTests();
	void OnStartTests(UINT nMenuID);
	void RunAllHistoryEntryTests(PRInt16);
	
protected:

};




