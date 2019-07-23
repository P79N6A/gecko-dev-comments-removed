










































#include "stdafx.h"
#include "testembed.h"
#include "nsIEditSession.h"
#include "QaUtils.h"
#include "BrowserFrm.h"
#include "BrowserImpl.h"
#include "BrowserView.h"
#include "Tests.h"
#include "nsIEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




CnsIEditSession::CnsIEditSession(nsIWebBrowser *mWebBrowser)
{
	qaWebBrowser = mWebBrowser;
}

CnsIEditSession::~CnsIEditSession()
{
}

nsIEditingSession * CnsIEditSession::GetEditSessionObject()
{
    editingSession = do_GetInterface(qaWebBrowser);
    if (!editingSession) {
        QAOutput("Didn't get nsIEditingSession object.");
		return nsnull;
	}

    return (editingSession);
}

void CnsIEditSession::InitTest(PRInt16 displayMode)
{
	editingSession = GetEditSessionObject();
	domWindow = GetTheDOMWindow(qaWebBrowser);
	if (editingSession) {
		rv = editingSession->Init(domWindow);
		RvTestResult(rv, "Init() test", displayMode);
		if (displayMode == 1)
			RvTestResultDlg(rv, "Init() test", true);
		if (!domWindow)
			QAOutput("Didn't get domWindow object for InitTest() test. Test failed.", displayMode);
	}
	else
		QAOutput("Didn't get editingSession object for InitTest() test. Test failed.", displayMode);
}

void CnsIEditSession::MakeWinEditTest(PRBool afterUriLoad, PRInt16 displayMode)
{
	editingSession = GetEditSessionObject();
	domWindow = GetTheDOMWindow(qaWebBrowser);
	if (editingSession) {
		rv= editingSession->MakeWindowEditable(domWindow, "text", afterUriLoad);
		RvTestResult(rv, "MakeWindowEditable() test", displayMode);
		if (displayMode == 1)
			RvTestResultDlg(rv, "MakeWindowEditable() test");
		if (!domWindow)
			QAOutput("Didn't get domWindow object for MakeWindowEditable() test. Test failed.", displayMode);
	}
	else
		QAOutput("Didn't get editingSession object for MakeWindowEditable() test. Test failed.", 1);
}

void CnsIEditSession::WinIsEditTest(PRBool outIsEditable, PRInt16 displayMode)
{
	editingSession = GetEditSessionObject();
	domWindow = GetTheDOMWindow(qaWebBrowser);
	if (editingSession) {
		rv = editingSession->WindowIsEditable(domWindow, &outIsEditable);
		RvTestResult(rv, "WindowIsEditable() test", displayMode);
		if (displayMode == 1)
			RvTestResultDlg(rv, "WindowIsEditable() test");
		if (!domWindow)
			QAOutput("Didn't get domWindow object for WindowIsEditable() test. Test failed.", displayMode);
		FormatAndPrintOutput("the outIsEditable boolean = ", outIsEditable, displayMode);
	}
	else
		QAOutput("Didn't get object(s) for WinIsEditTest() test. Test failed.", 1);
}

void CnsIEditSession::GetEditorWinTest(PRInt16 displayMode)
{
	nsCOMPtr<nsIEditor> theEditor;

	editingSession = GetEditSessionObject();
	domWindow = GetTheDOMWindow(qaWebBrowser);
	if (editingSession) {
		rv = editingSession->GetEditorForWindow(domWindow, getter_AddRefs(theEditor));
		RvTestResult(rv, "GetEditorForWindow() test", displayMode);
		if (displayMode == 1)
			RvTestResultDlg(rv, "GetEditorForWindow() test");
		if (!domWindow)
			QAOutput("Didn't get domWindow object for GetEditorForWindow() test. Test failed.", displayMode);

		if (!theEditor) 
			QAOutput("Didn't get the Editor object.");
	}
	else
		QAOutput("Didn't get object(s) for WinIsEditTest() test. Test failed.", 1);
}

void CnsIEditSession::SetEditorWinTest(PRInt16 displayMode)
{
	editingSession = GetEditSessionObject();
	domWindow = GetTheDOMWindow(qaWebBrowser);
	if (editingSession) {
		rv = editingSession->SetupEditorOnWindow(domWindow);
		RvTestResult(rv, "SetupEditorOnWindow() test", displayMode);
		if (displayMode == 1)
			RvTestResultDlg(rv, "SetupEditorOnWindow() test");
		if (!domWindow)
			QAOutput("Didn't get domWindow object for SetupEditorOnWindow() test. Test failed.", displayMode);
	}
	else
		QAOutput("Didn't get object(s) for SetEditorWinTest() test. Test failed.", 1);
}

void CnsIEditSession::TearEditorWinTest(PRInt16 displayMode)
{
	editingSession = GetEditSessionObject();
	domWindow = GetTheDOMWindow(qaWebBrowser);
	if (editingSession) {
		rv = editingSession->TearDownEditorOnWindow(domWindow);
		RvTestResult(rv, "TearDownEditorOnWindow() test", displayMode);
		if (displayMode == 1)
			RvTestResultDlg(rv, "TearDownEditorOnWindow() test");
		if (!domWindow)
			QAOutput("Didn't get domWindow object for TearDownEditorOnWindow() test. Test failed.", displayMode);
	}
	else
		QAOutput("Didn't get object(s) for TearEditorWinTest() test. Test failed.", 1);
}

void CnsIEditSession::OnStartTests(UINT nMenuID)
{
	switch(nMenuID)
	{
		case ID_INTERFACES_NSIEDITINGSESSION_RUNALLTESTS :
			RunAllTests();
			break;
		case ID_INTERFACES_NSIEDITINGSESSION_INIT :
			InitTest(2);
			break;
		case ID_INTERFACES_NSIEDITINGSESSION_MAKEWINDOWEDITABLE :
			MakeWinEditTest(PR_FALSE, 2);
			break;
		case ID_INTERFACES_NSIEDITINGSESSION_WINDOWISEDITABLE  :
			WinIsEditTest(PR_TRUE, 2);
			break;
		case ID_INTERFACES_NSIEDITINGSESSION_GETEDITORFORWINDOW :
			GetEditorWinTest(2);
			break;
		case ID_INTERFACES_NSIEDITINGSESSION_SETUPEDITORONWINDOW :
			SetEditorWinTest(2);
			break;
		case ID_INTERFACES_NSIEDITINGSESSION_TEARDOWNEDITORONWINDOW :
			TearEditorWinTest(2);
			break;
	}
}

void CnsIEditSession::RunAllTests()
{
	InitTest(1);
	MakeWinEditTest(PR_FALSE, 1);
	WinIsEditTest(PR_TRUE, 1);
	GetEditorWinTest(1);

	TearEditorWinTest(1);
}



