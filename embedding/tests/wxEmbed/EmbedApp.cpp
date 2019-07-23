





























#include "global.h"

#include "nsIProfile.h"
#include "nsIWindowWatcher.h"
#include "nsMemory.h"
#include "nsEmbedString.h"

#include "BrowserFrame.h"
#include "MailFrame.h"
#include "ChatFrame.h"
#include "EditorFrame.h"

#include "GeckoProtocolHandler.h"
#include "GeckoWindowCreator.h"

class FooCallback : public GeckoChannelCallback
{
public:
    virtual nsresult GetData(
        nsIURI *aURI,
        nsIChannel *aChannel,
        nsACString &aContentType,
        void **aData,
        PRUint32 *aSize)
    {
        nsCAutoString spec;
        nsCAutoString txt("<html><body>Hello, your URL was \'");
        aURI->GetSpec(spec);
        txt.Append(spec);
        txt.Append("\'</body></html>");
        aContentType.Assign("text/html");

        size_t size = txt.Length();
        *aData = (void *) nsMemory::Alloc(size + 1);
        if (!*aData)
            return NS_ERROR_OUT_OF_MEMORY;
        memset(*aData, 0, size + 1);
        memcpy(*aData, txt.get(), size);
        *aSize = size;
        return NS_OK;
    }
};

class EmbedApp : public wxApp
{
    virtual bool OnInit();
    virtual int OnExit();

    DECLARE_EVENT_TABLE()
    void OnQuit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnMail(wxCommandEvent &event);
    void OnChat(wxCommandEvent &event);
    void OnEditor(wxCommandEvent &event);
};

BEGIN_EVENT_TABLE(EmbedApp, wxApp)
    EVT_MENU(XRCID("menu_quit"),   EmbedApp::OnQuit)
    EVT_MENU(XRCID("menu_about"),  EmbedApp::OnAbout)
    EVT_MENU(XRCID("menu_mail"),   EmbedApp::OnMail)
    EVT_MENU(XRCID("menu_chat"),   EmbedApp::OnChat)
    EVT_MENU(XRCID("menu_editor"), EmbedApp::OnEditor)
END_EVENT_TABLE()



IMPLEMENT_APP(EmbedApp)

bool EmbedApp::OnInit()
{
    wxImage::AddHandler(new wxGIFHandler);
    wxXmlResource::Get()->InitAllHandlers();
    InitXmlResource();

    if (NS_FAILED(NS_InitEmbedding(nsnull, nsnull)))
    {
        return FALSE;
    }

    nsresult rv;
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    rv = profileService->CreateNewProfile(L"wxEmbed", nsnull, nsnull, PR_FALSE);
    if (NS_FAILED(rv)) return FALSE;
    rv = profileService->SetCurrentProfile(L"wxEmbed");
    if (NS_FAILED(rv)) return FALSE;

    GeckoProtocolHandler::RegisterHandler("foo", "Test handler", new FooCallback);

    
    GeckoWindowCreator *creatorCallback = new GeckoWindowCreator();
    if (creatorCallback)
    {
        nsCOMPtr<nsIWindowCreator> windowCreator(NS_STATIC_CAST(nsIWindowCreator *, creatorCallback));
        if (windowCreator)
        {
            nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
            if (wwatch)
            {
                wwatch->SetWindowCreator(windowCreator);
            }
        }
    }

    
    BrowserFrame* frame = new BrowserFrame(NULL);
    frame->Show(TRUE);
    

    SetTopWindow(frame);

    return TRUE;
}

int EmbedApp::OnExit()
{
    
    nsresult rv;
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
    {
        profileService->ShutDownCurrentProfile(nsIProfile::SHUTDOWN_PERSIST); 
    }
    NS_TermEmbedding();

    return 0;
}

void EmbedApp::OnQuit(wxCommandEvent & WXUNUSED(event))
{
    wxFrame * frame = NULL;
    


















    GetTopWindow()->Close(TRUE);
}

void EmbedApp::OnAbout(wxCommandEvent & WXUNUSED(event))
{
    (void)wxMessageBox(_T("wxEmbed"), _T("About wxEmbed"));
}

void EmbedApp::OnMail(wxCommandEvent & WXUNUSED(event))
{
    
    MailFrame * frame = (MailFrame *) wxWindow::FindWindowByName("mail", NULL);;
    if (!frame)
        frame = new MailFrame(NULL);
    if (frame)
        frame->Show(TRUE);
}

void EmbedApp::OnChat(wxCommandEvent & WXUNUSED(event))
{
    ChatFrame * frame = (ChatFrame *) wxWindow::FindWindowByName("chat", NULL);
    if (!frame)
        frame = new ChatFrame(NULL);
    if (frame)
        frame->Show(TRUE);
}

void EmbedApp::OnEditor(wxCommandEvent & WXUNUSED(event))
{
    EditorFrame * frame = (EditorFrame *) wxWindow::FindWindowByName("editor", NULL);
    if (!frame)
        frame = new EditorFrame(NULL);
    if (frame)
        frame->Show(TRUE);
}
