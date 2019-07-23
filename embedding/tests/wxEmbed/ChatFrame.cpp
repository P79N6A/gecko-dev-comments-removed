





























#include "global.h"

#include "wx/strconv.h"

#include "nsIWebNavigation.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"

#include "ChatFrame.h"

BEGIN_EVENT_TABLE(ChatFrame, GeckoFrame)
    EVT_TEXT_ENTER(XRCID("chat"),        ChatFrame::OnChat)
END_EVENT_TABLE()

ChatFrame::ChatFrame(wxWindow* aParent)
{
    wxXmlResource::Get()->LoadFrame(this, aParent, wxT("chat_frame"));

    SetName("chat");

    SetIcon(wxICON(appicon));

    SetupDefaultGeckoWindow();

    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
    webNav->LoadURI(NS_ConvertASCIItoUTF16("about:blank").get(),
        nsIWebNavigation::LOAD_FLAGS_NONE, nsnull, nsnull, nsnull);

}

void ChatFrame::OnChat(wxCommandEvent &event)
{
    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
    nsCOMPtr<nsIDOMDocument> doc;
    webNav->GetDocument(getter_AddRefs(doc));
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(doc);
    

    wxTextCtrl *chatCtrl = (wxTextCtrl *) FindWindowById(XRCID("chat"), this);
    if (chatCtrl)
    {
        wxString html("<p>Foo: ");
        html += chatCtrl->GetValue();
        wxMBConv conv;
        nsAutoString htmlU(conv.cWX2WC(html));
        htmlDoc->Writeln(htmlU);
        chatCtrl->SetValue("");
    }
}
