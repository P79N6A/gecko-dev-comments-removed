











































#if !defined(AFX_NSIWEBNAV_H__4E002235_7569_11D5_89E7_00010316305A__INCLUDED_)
#define AFX_NSIWEBNAV_H__4E002235_7569_11D5_89E7_00010316305A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 





class CNsIWebNav 
{

public:
	CNsIWebNav(nsIWebNavigation *mWebNav);


public:
	nsCOMPtr<nsIWebNavigation> qaWebNav;

public:

	
	void CanGoBackTest(PRInt16);
	void GoBackTest(PRInt16);
	void CanGoForwardTest(PRInt16);
	void GoForwardTest(PRInt16);
	void GoToIndexTest(PRInt16);
	void LoadURITest(char *, PRUint32, PRInt16 displayMode=1, 
					 PRBool runAllTests=PR_FALSE);
	void ReloadTest(PRUint32, PRInt16);
	void StopURITest(char *, PRUint32, PRInt16);
	void GetDocumentTest(PRInt16);
	void GetCurrentURITest(PRInt16);
	void GetReferringURITest(PRInt16);
	void GetSHTest(PRInt16);
	void SetSHTest(PRInt16);
	void LoadUriandReload(int);
	void OnStartTests(UINT nMenuID);
	void RunAllTests();



public:
	virtual ~CNsIWebNav();

	
protected:
};

typedef struct
{
  char			theURI[1024];
  unsigned long theFlag;
} NavElement;






#endif
