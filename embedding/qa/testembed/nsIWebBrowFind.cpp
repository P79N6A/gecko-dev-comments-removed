









































#include "Stdafx.h"
#include "TestEmbed.h"
#include "BrowserImpl.h"
#include "BrowserFrm.h"
#include "Tests.h"
#include "domwindow.h"
#include "QaUtils.h"
#include <stdio.h>
#include "nsIWebBrowFind.h"
#include "UrlDialog.h"
#include "QaFindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CNsIWebBrowFind::CNsIWebBrowFind(nsIWebBrowser *mWebBrowser, CBrowserImpl *mpBrowserImpl)
{
	qaWebBrowser = mWebBrowser;
	qaBrowserImpl = mpBrowserImpl;
}


CNsIWebBrowFind::~CNsIWebBrowFind()
{

}

	
nsIWebBrowserFind * CNsIWebBrowFind::GetWebBrowFindObject()
{
	nsCOMPtr<nsIWebBrowserFind> qaWBFind(do_GetInterface(qaWebBrowser, &rv));
	if (!qaWBFind) {
		QAOutput("Didn't get WebBrowserFind object.", 2);
		return NULL;
	}
	else {
		RvTestResult(rv, "nsIWebBrowserFind object test", 1);
		RvTestResultDlg(rv, "nsIWebBrowserFind object test");
		return(qaWBFind);
	}
}

void CNsIWebBrowFind::SetSearchStringTest(PRInt16 displayMode)
{
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();

	nsString searchString;
	if (myDialog.DoModal() == IDOK) {

		
		searchString.AssignWithConversion(myDialog.m_textfield);
		rv = qaWBFind->SetSearchString(searchString.get());
	}
	RvTestResult(rv, "nsIWebBrowserFind::SetSearchString() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::SetSearchString test");
}

void CNsIWebBrowFind::GetSearchStringTest(PRInt16 displayMode)
{
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();

	
	nsXPIDLString stringBuf;
	CString csSearchStr;
	rv = qaWBFind->GetSearchString(getter_Copies(stringBuf));
	RvTestResult(rv, "nsIWebBrowserFind::GetSearchString() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::GetSearchString() test");
	csSearchStr = stringBuf.get();
	FormatAndPrintOutput("The searched string = ", csSearchStr, displayMode);
}

void CNsIWebBrowFind::FindNextTest(PRBool didFind, PRInt16 displayMode)
{
	

	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();

	rv = qaWBFind->FindNext(&didFind);
	RvTestResult(rv, "nsIWebBrowserFind::FindNext() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::FindNext() test");
	FormatAndPrintOutput("returned didFind = ", didFind, displayMode);
}

void CNsIWebBrowFind::SetFindBackwardsTest(PRBool didFindBackwards, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->SetFindBackwards(didFindBackwards);
	RvTestResult(rv, "nsIWebBrowserFind::SetFindBackwards() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::SetFindBackwards() test", displayMode);
}

void CNsIWebBrowFind::GetFindBackwardsTest(PRBool didFindBackwards, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->GetFindBackwards(&didFindBackwards);
	RvTestResult(rv, "nsIWebBrowserFind::GetFindBackwards() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::GetFindBackwards() test");
	FormatAndPrintOutput("returned didFindBackwards = ", didFindBackwards, displayMode);
}

void CNsIWebBrowFind::SetWrapFindTest(PRBool didWrapFind, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->SetWrapFind(didWrapFind);
	RvTestResult(rv, "nsIWebBrowserFind::SetWrapFind() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::SetWrapFind() test");
}

void CNsIWebBrowFind::GetWrapFindTest(PRBool didWrapFind, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->GetWrapFind(&didWrapFind);
	RvTestResult(rv, "nsIWebBrowserFind::GetWrapFind() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::GetWrapFind() test");
	FormatAndPrintOutput("returned didWrapFind = ", didWrapFind, displayMode);
}

void CNsIWebBrowFind::SetEntireWordTest(PRBool didEntireWord, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->SetEntireWord(didEntireWord);
	RvTestResult(rv, "nsIWebBrowserFind::SetEntireWord() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::SetEntireWord() test");
}

void CNsIWebBrowFind::GetEntireWordTest(PRBool didEntireWord, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->GetEntireWord(&didEntireWord);
	RvTestResult(rv, "nsIWebBrowserFind::GetEntireWord() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::GetEntireWord() test");
	FormatAndPrintOutput("returned didEntireWord = ", didEntireWord, displayMode);
}

void CNsIWebBrowFind::SetMatchCase(PRBool didMatchCase, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->SetMatchCase(didMatchCase);
	RvTestResult(rv, "nsIWebBrowserFind::SetMatchCase() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::SetMatchCase() test");
}

void CNsIWebBrowFind::GetMatchCase(PRBool didMatchCase, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->GetMatchCase(&didMatchCase);
	RvTestResult(rv, "nsIWebBrowserFind::GetMatchCase() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::GetMatchCase() test");
	FormatAndPrintOutput("returned didMatchCase = ", didMatchCase, displayMode);
}

void CNsIWebBrowFind::SetSearchFrames(PRBool didSearchFrames, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->SetSearchFrames(didSearchFrames);
	RvTestResult(rv, "nsIWebBrowserFind::SetSearchFrames() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::SetSearchFrames() test");
}

void CNsIWebBrowFind::GetSearchFrames(PRBool didSearchFrames, PRInt16 displayMode)
{
	
	nsCOMPtr<nsIWebBrowserFind> qaWBFind;
	qaWBFind = GetWebBrowFindObject();
	
	rv = qaWBFind->GetSearchFrames(&didSearchFrames);
	RvTestResult(rv, "nsIWebBrowserFind::GetSearchFrames() test", displayMode);
	RvTestResultDlg(rv, "nsIWebBrowserFind::GetSearchFrames() test");
	FormatAndPrintOutput("returned didSearchFrames = ", didSearchFrames, displayMode);
}

void CNsIWebBrowFind::OnStartTests(UINT nMenuID)
{
	switch(nMenuID)
	{
		case ID_INTERFACES_NSIWEBBROWSERFIND_RUNALLTESTS :
			RunAllTests();
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_SETSEARCHSTRINGTEST :
			SetSearchStringTest(2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_GETSEARCHSTRINGTEST :
			GetSearchStringTest(2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_FINDNEXTTEST  :
			FindNextTest(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_SETFINDBACKWARDSTEST :
			SetFindBackwardsTest(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_GETFINDBACKWARDSTEST :
			GetFindBackwardsTest(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_SETWRAPFINDTEST :
			SetWrapFindTest(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_GETWRAPFINDTEST  :
			GetWrapFindTest(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_SETENTIREWORDTEST  :
			SetEntireWordTest(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_GETENTIREWORDTEST :
			GetEntireWordTest(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_SETMATCHCASE :
			SetMatchCase(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_GETMATCHCASE :
			GetMatchCase(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_SETSEARCHFRAMES  :
			SetSearchFrames(PR_TRUE, 2);
			break ;
		case ID_INTERFACES_NSIWEBBROWSERFIND_GETSEARCHFRAMES  :
			GetSearchFrames(PR_TRUE, 2);
			break ;
	}
}

void CNsIWebBrowFind::RunAllTests()
{
	QAOutput("Begin WebBrowserFind tests.", 1);
	GetWebBrowFindObject();
	SetSearchStringTest(1);
	GetSearchStringTest(1);
	FindNextTest(PR_TRUE, 1);
	FindNextTest(PR_TRUE, 1);  
	SetFindBackwardsTest(PR_TRUE, 1);
	GetFindBackwardsTest(PR_TRUE, 1);
	FindNextTest(PR_TRUE, 1);	
	SetWrapFindTest(PR_TRUE, 1);
	GetWrapFindTest(PR_TRUE, 1);
	FindNextTest(PR_TRUE, 1);  
	SetEntireWordTest(PR_TRUE, 1);
	GetEntireWordTest(PR_TRUE, 1);
	FindNextTest(PR_TRUE, 1);  
	SetMatchCase(PR_TRUE, 1);
	GetMatchCase(PR_TRUE, 1);
	FindNextTest(PR_TRUE, 1);  
	SetSearchFrames(PR_TRUE, 1);
	GetSearchFrames(PR_TRUE, 1);
	FindNextTest(PR_TRUE, 1);  

	QAOutput("PR_FALSE tests", 1);
	SetFindBackwardsTest(PR_FALSE, 1);
	GetFindBackwardsTest(PR_FALSE, 1);
	FindNextTest(PR_FALSE, 1);		
	SetWrapFindTest(PR_FALSE, 1);
	GetWrapFindTest(PR_FALSE, 1);
	FindNextTest(PR_FALSE, 1);		
	SetEntireWordTest(PR_FALSE, 1);
	GetEntireWordTest(PR_FALSE, 1);
	FindNextTest(PR_FALSE, 1);		
	SetMatchCase(PR_FALSE, 1);
	GetMatchCase(PR_FALSE, 1);
	FindNextTest(PR_FALSE, 1);		
	SetSearchFrames(PR_FALSE, 1);
	GetSearchFrames(PR_FALSE, 1);
	FindNextTest(PR_FALSE, 1);		
	QAOutput("End WebBrowserFind tests.", 1);
}