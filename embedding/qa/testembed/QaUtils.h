










































#ifndef _QAUTILS_H
#define _QAUTILS_H

#if _MSC_VER > 1000
	#pragma once
#endif

#include "BrowserView.h"
#include "resource.h"
#include "stdafx.h"

extern void RvTestResultDlg(nsresult rv, CString pLine,BOOL bClearList = false);
extern void RvTestResult(nsresult, const char *, int displayMethod=1);
extern void WriteToOutputFile(const char *);
extern void QAOutput(const char *pLine, int displayMethod=1);
extern void FormatAndPrintOutput(const char *, const char *, int);
extern void FormatAndPrintOutput(const char *, nsCAutoString, int);
extern void FormatAndPrintOutput(const char *, int, int);
extern void FormatAndPrintOutput(const char *, double, int);
extern void FormatAndPrintOutput(const char *, PRUint32, int);
extern void RequestName(nsIRequest *, nsCString &, int displayMethod=1);
extern void WebProgDOMWindowTest(nsIWebProgress *, const char *,int displayMethod=1);
extern void WebProgIsDocLoadingTest(nsIWebProgress *, const char *, int displayMethod=1);
extern void SaveObject(nsISupports *theSupports);
extern nsIDOMWindow * GetTheDOMWindow(nsIWebBrowser *);
extern nsCAutoString GetTheURI(nsIURI *theURI, int displayMethod=1);
extern void onStateChangeString(char *, char *, nsCString, PRUint32, int displayMethod=1);
extern nsresult rv;

#endif 



class CShowTestResults : public CDialog
{

public:
	CShowTestResults(CWnd* pParent = NULL);   
	void AddItemToList(LPCTSTR szTestCaseName, BOOL bResult);


	
	enum { IDD = IDD_RUNTESTSDLG };
	CListCtrl	m_ListResults;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


private:
	LPCTSTR m_TitleString ;
protected:

	
	
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()
};


typedef struct {
	nsCOMPtr<nsISupports> sup;
}storage;


