










































#include "stdafx.h"
#include "QaUtils.h"
#include <stdio.h>
#include "nsIClipboardCmd.h"

CNsIClipBoardCmd::CNsIClipBoardCmd(nsIWebBrowser* mWebBrowser)
{
	qaWebBrowser = mWebBrowser;
}


CNsIClipBoardCmd::~CNsIClipBoardCmd()
{
}


void CNsIClipBoardCmd::OnStartTests(UINT nMenuID)
{
	
	

	switch(nMenuID)
	{
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_PASTE	:
			OnPasteTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_COPYSELECTION :
			OnCopyTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_SELECTALL :
			OnSelectAllTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_SELECTNONE :
			OnSelectNoneTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_CUTSELECTION :
			OnCutSelectionTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_COPYLINKLOCATION :
			copyLinkLocationTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_CANCOPYSELECTION :
			canCopySelectionTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_CANCUTSELECTION :
			canCutSelectionTest();
			break ;
		case ID_INTERFACES_NSICLIPBOARDCOMMANDS_CANPASTE :
			canPasteTest();
			break ;
	}

}




void CNsIClipBoardCmd::OnPasteTest()
{
    QAOutput("testing paste command", 1);
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
        rv = clipCmds->Paste();
		RvTestResult(rv, "nsIClipboardCommands::Paste()' rv test", 1);

	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::OnCopyTest()
{
    QAOutput("testing copyselection command");
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
        rv = clipCmds->CopySelection();
		RvTestResult(rv, "nsIClipboardCommands::CopySelection()' rv test", 1);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::OnSelectAllTest()
{
    QAOutput("testing selectall method");
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
        rv = clipCmds->SelectAll();
		RvTestResult(rv, "nsIClipboardCommands::SelectAll()' rv test", 1);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::OnSelectNoneTest()
{
    QAOutput("testing selectnone method");
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
        rv = clipCmds->SelectNone();
		RvTestResult(rv, "nsIClipboardCommands::SelectNone()' rv test", 1);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::OnCutSelectionTest()
{
    QAOutput("testing cutselection method");
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
        rv = clipCmds->CutSelection();
		RvTestResult(rv, "nsIClipboardCommands::CutSelection()' rv test", 1);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::copyLinkLocationTest()
{
    QAOutput("testing CopyLinkLocation method", 2);
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
        rv = clipCmds->CopyLinkLocation();
		RvTestResult(rv, "nsIClipboardCommands::CopyLinkLocation()' rv test", 1);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::canCopySelectionTest()
{
    PRBool canCopySelection = PR_FALSE;
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
       rv = clipCmds->CanCopySelection(&canCopySelection);
	   RvTestResult(rv, "nsIClipboardCommands::CanCopySelection()' rv test", 1);

       if(canCopySelection)
          QAOutput("The selection you made Can be copied", 2);
       else
          QAOutput("Either you did not make a selection or The selection you made Cannot be copied", 2);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::canCutSelectionTest()
{
    PRBool canCutSelection = PR_FALSE;
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
       rv = clipCmds->CanCutSelection(&canCutSelection);
	   RvTestResult(rv, "nsIClipboardCommands::CanCutSelection()' rv test", 1);

	   if(canCutSelection)
          QAOutput("The selection you made Can be cut", 2);
       else
          QAOutput("Either you did not make a selection or The selection you made Cannot be cut", 2);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


void CNsIClipBoardCmd::canPasteTest()
{
    PRBool canPaste = PR_FALSE;
    nsCOMPtr<nsIClipboardCommands> clipCmds = do_GetInterface(qaWebBrowser);
    if (clipCmds)
	{
        rv = clipCmds->CanPaste(&canPaste);
	    RvTestResult(rv, "nsIClipboardCommands::CanPaste()' rv test", 1);

		if(canPaste)
			QAOutput("The clipboard contents can be pasted here", 2);
		else
			QAOutput("The clipboard contents cannot be pasted here", 2);
	}
	else
		QAOutput("We didn't get the clipboard object.", 1);
}


