




























 
#include "global.h"

#include "nsIWebNavigation.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMWindow.h"
#include "nsIEditingSession.h"

#include "EditorFrame.h"

BEGIN_EVENT_TABLE(EditorFrame, GeckoFrame)
    EVT_MENU(XRCID("edit_bold"),           EditorFrame::OnEditBold)
    EVT_MENU(XRCID("edit_italic"),         EditorFrame::OnEditItalic)
    EVT_MENU(XRCID("edit_underline"),      EditorFrame::OnEditUnderline)
    EVT_UPDATE_UI(XRCID("edit_bold"),      EditorFrame::OnUpdateToggleCmd)
    EVT_UPDATE_UI(XRCID("edit_italic"),    EditorFrame::OnUpdateToggleCmd)
    EVT_UPDATE_UI(XRCID("edit_underline"), EditorFrame::OnUpdateToggleCmd)
    EVT_MENU(XRCID("edit_indent"),         EditorFrame::OnEditIndent)
    EVT_MENU(XRCID("edit_outdent"),        EditorFrame::OnEditOutdent)
    EVT_MENU(XRCID("edit_aleft"),          EditorFrame::OnEditAlignLeft)
    EVT_MENU(XRCID("edit_acenter"),        EditorFrame::OnEditAlignCenter)
    EVT_MENU(XRCID("edit_aright"),         EditorFrame::OnEditAlignRight)
END_EVENT_TABLE()

EditorFrame::EditorFrame(wxWindow* aParent)
{
    wxXmlResource::Get()->LoadFrame(this, aParent, wxT("editor_frame"));

    SetName("editor");

    SetIcon(wxICON(appicon));

    SetupDefaultGeckoWindow();

    SendSizeEvent(); 
    
    CreateStatusBar();

    MakeEditable();

    mCommandManager = do_GetInterface(mWebBrowser);

    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
    webNav->LoadURI(NS_ConvertASCIItoUTF16("www.mozilla.org").get(),
        nsIWebNavigation::LOAD_FLAGS_NONE, nsnull, nsnull, nsnull);

}

void EditorFrame::MakeEditable()
{
    nsCOMPtr<nsIDOMWindow> domWindow;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));

    nsCOMPtr<nsIEditingSession> editingSession = do_GetInterface(mWebBrowser);
    if (!editingSession)
        return;
    editingSession->MakeWindowEditable(domWindow, NULL, PR_TRUE, PR_TRUE,
                                       PR_FALSE);
}

nsresult EditorFrame::DoCommand(const char *aCommand, nsICommandParams *aCommandParams)
{
    if (mCommandManager)
    {
        nsCOMPtr<nsIDOMWindow> domWindow;
        mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
        return mCommandManager->DoCommand(aCommand, aCommandParams, domWindow);
    }
    return NS_ERROR_FAILURE;
}

void EditorFrame::IsCommandEnabled(const char *aCommand, PRBool *retval)
{
    if (mCommandManager)
    {
        nsCOMPtr<nsIDOMWindow> domWindow;
        mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
        mCommandManager->IsCommandEnabled(aCommand, domWindow, retval);
    }
}

void EditorFrame::GetCommandState(const char *aCommand, nsICommandParams *aCommandParams)
{
    if (mCommandManager)
    {
        nsCOMPtr<nsIDOMWindow> domWindow;
        mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
        mCommandManager->GetCommandState(aCommand, domWindow, aCommandParams);
    }
}

void EditorFrame::UpdateStatusBarText(const PRUnichar* aStatusText)
{
    SetStatusText(aStatusText);
}

static
nsresult MakeCommandParams(nsICommandParams **aParams)
{
    nsresult rv;
    nsCOMPtr<nsICommandParams> params = do_CreateInstance(NS_COMMAND_PARAMS_CONTRACTID,&rv);
    if (NS_FAILED(rv))
        return rv;
    if (!params)
        return NS_ERROR_FAILURE;
    *aParams = params;
    NS_ADDREF(*aParams);
    return rv;
}

static const char kCmdBold[]      = "cmd_bold";
static const char kCmdItalic[]    = "cmd_italic";
static const char kCmdUnderline[] = "cmd_underline";
static const char kCmdIndent[]    = "cmd_indent";
static const char kCmdOutdent[]   = "cmd_outdent";
static const char kCmdAlign[]     = "cmd_align";

void EditorFrame::OnEditBold(wxCommandEvent &event)
{
    DoCommand(kCmdBold, nsnull);
}

void EditorFrame::OnEditItalic(wxCommandEvent &event)
{
    DoCommand(kCmdItalic, nsnull);
}

void EditorFrame::OnEditUnderline(wxCommandEvent &event)
{
    DoCommand(kCmdUnderline, nsnull);
}

void EditorFrame::OnEditIndent(wxCommandEvent &event)
{
    DoCommand(kCmdIndent, nsnull);
}

void EditorFrame::OnEditOutdent(wxCommandEvent &event)
{
    DoCommand(kCmdOutdent, nsnull);
}

const char kAlignLeft[]   = "left";
const char kAlignRight[]  = "right";
const char kAlignCenter[] = "center";


void EditorFrame::OnEditAlignLeft(wxCommandEvent &event)
{
    ExecuteAttribParam(kCmdAlign, kAlignLeft);
}

void EditorFrame::OnEditAlignRight(wxCommandEvent &event)
{
    ExecuteAttribParam(kCmdAlign, kAlignRight);
}

void EditorFrame::OnEditAlignCenter(wxCommandEvent &event)
{
    ExecuteAttribParam(kCmdAlign, kAlignCenter);
}

const char kStateAll[]       = "state_all";
const char kStateMixed[]     = "state_mixed";
const char kStateAttribute[] = "state_attribute";
const char kStateEnabled[]   = "state_enabled";

void EditorFrame::OnUpdateToggleCmd(wxUpdateUIEvent &event)
{
    const char *cmd = NULL;
    if (event.GetId() == XRCID("edit_bold"))
    {
        cmd = kCmdBold;
    }
    else if (event.GetId() == XRCID("edit_italic"))
    {
        cmd = kCmdItalic;
    }
    else if (event.GetId() == XRCID("edit_underline"))
    {
        cmd = kCmdUnderline;
    }

    
    nsCOMPtr<nsICommandParams> params;
    if (NS_SUCCEEDED(MakeCommandParams(getter_AddRefs(params))))
    {
        GetCommandState(cmd, params);

        
        
        
        
        PRBool bMixedStyle = PR_FALSE;
        params->GetBooleanValue(kStateMixed, &bMixedStyle);

        
        
        
        PRBool bCmdEnabled = PR_FALSE;
        params->GetBooleanValue(kStateAll, &bCmdEnabled);

        event.Check((bMixedStyle || bCmdEnabled) ? TRUE : FALSE);
    }
}

nsresult EditorFrame::ExecuteAttribParam(const char *aCommand, const char *aAttribute)
{
    nsresult rv;
    nsCOMPtr<nsICommandParams> params;
    rv = MakeCommandParams(getter_AddRefs(params));
    if (NS_FAILED(rv))
        return rv;
    if (!params)
        return NS_ERROR_FAILURE;
    params->SetCStringValue(kStateAttribute, aAttribute);
    return DoCommand(aCommand, params);
}
