




































#ifndef webshell____h
#define webshell____h

#include "nsError.h"
#include "nsIWebShellServices.h"
#include "nsILinkHandler.h"
#include "nsIClipboardCommands.h"
#include "nsDocShell.h"
#include "nsICommandManager.h"
#include "nsIIOService.h"
#include "nsCRT.h"

class nsIController;
struct PRThread;
struct OnLinkClickEvent;

typedef enum {
    eCharsetReloadInit,
    eCharsetReloadRequested,
    eCharsetReloadStopOrigional
} eCharsetReloadState;

#define NS_ERROR_WEBSHELL_REQUEST_REJECTED  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL,1001)


#define NS_WEB_SHELL_CID \
 { 0xa6cf9059, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsWebShell : public nsDocShell,
                   public nsIWebShellServices,
                   public nsILinkHandler,
                   public nsIClipboardCommands
{
public:
    nsWebShell();
    virtual ~nsWebShell();

    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICLIPBOARDCOMMANDS
    NS_DECL_NSIWEBSHELLSERVICES

    
    NS_IMETHOD OnLinkClick(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        nsIInputStream* aPostDataStream = 0,
        nsIInputStream* aHeadersDataStream = 0);
    NS_IMETHOD OnLinkClickSync(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        nsIInputStream* aPostDataStream = 0,
        nsIInputStream* aHeadersDataStream = 0,
        nsIDocShell** aDocShell = 0,
        nsIRequest** aRequest = 0);
    NS_IMETHOD OnOverLink(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec);
    NS_IMETHOD OnLeaveLink();
    NS_IMETHOD GetLinkState(nsIURI* aLinkURI, nsLinkState& aState);

    NS_IMETHOD Create();

    static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);

    

    friend struct OnLinkClickEvent;

protected:
    void InitFrameData();

    
    virtual nsresult GetControllerForCommand ( const char *inCommand, nsIController** outController );
    virtual nsresult IsCommandEnabled ( const char * inCommand, PRBool* outEnabled );
    virtual nsresult DoCommand ( const char * inCommand );
    nsresult EnsureCommandHandler();

    
    
    
    
    virtual nsresult EndPageLoad(nsIWebProgress *aProgress,
        nsIChannel* channel,
        nsresult aStatus);

    eCharsetReloadState mCharsetReloadState;

    nsresult CreateViewer(nsIRequest* request,
        const char* aContentType,
        const char* aCommand,
        nsIStreamListener** aResult);

    nsCOMPtr<nsICommandManager> mCommandManager;
    
#ifdef DEBUG
private:
    
    static unsigned long gNumberOfWebShells;
#endif 
};

#endif 

