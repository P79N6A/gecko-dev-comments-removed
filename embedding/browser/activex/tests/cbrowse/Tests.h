



































#ifndef TESTS_H
#define TESTS_H

#include "CBrowse_i.h"

class CBrowseDlg;
struct Test;

class BrowserInfo
{
public:
	Test *pTest;
	TestResult nResult;
	CBrowserCtlSiteInstance *pControlSite;
	IUnknown *pIUnknown;
	CLSID clsid;
	CBrowseDlg *pBrowseDlg;
	CString szTestURL;
	CString szTestCGI;

	void OutputString(const TCHAR *szMessage, ...);
	HRESULT GetWebBrowser(IWebBrowserApp **pWebBrowser);
	HRESULT GetDocument(IHTMLDocument2 **pDocument);
};


typedef TestResult (__cdecl *TestProc)(BrowserInfo &cInfo);

struct Test
{
	TCHAR szName[256];
	TCHAR szDesc[256];
	TestProc pfn;
	TestResult nLastResult;
};

struct TestSet;
typedef void (__cdecl *SetPopulatorProc)(TestSet *pTestSet);

struct TestSet
{
	TCHAR *szName;
	TCHAR *szDesc;
	int    nTests;
	Test  *aTests;
	SetPopulatorProc pfnPopulator;
};

extern TestSet aTestSets[];
extern int nTestSets;

#endif