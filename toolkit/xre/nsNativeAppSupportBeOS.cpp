



































 


# ifdef DC_PROGRAMNAME
#include <DebugConsole.h>
#endif

#include "nsIServiceManager.h"
#include "nsNativeAppSupportBase.h"
#include "nsICommandLineRunner.h"
#include "nsCOMPtr.h"
#include "nsIProxyObjectManager.h"

#include "nsPIDOMWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsIWindowMediator.h"
#include "nsXPIDLString.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIDocShell.h"

#include <Application.h>
#include <AppFileInfo.h>
#include <Resources.h>
#include <Path.h>
#include <Window.h>
#include <unistd.h>


static nsresult
GetMostRecentWindow(const PRUnichar* aType, nsIDOMWindowInternal** aWindow)
{
    nsresult rv;
    nsCOMPtr<nsIWindowMediator> med(do_GetService( NS_WINDOWMEDIATOR_CONTRACTID, &rv));
    if (NS_FAILED(rv))
        return rv;

    if (med)
    {
        nsCOMPtr<nsIWindowMediator> medProxy;
        rv = NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, NS_GET_IID(nsIWindowMediator), 
                                  med, NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(medProxy));
        if (NS_FAILED(rv))
            return rv;
        return medProxy->GetMostRecentWindow( aType, aWindow );
    }
    return NS_ERROR_FAILURE;
}

static nsresult
ActivateWindow(nsIDOMWindowInternal* aWindow)
{
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
    NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
    nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(window->GetDocShell()));
    NS_ENSURE_TRUE(baseWindow, NS_ERROR_FAILURE);
    nsCOMPtr<nsIWidget> mainWidget;
    baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
    NS_ENSURE_TRUE(mainWidget, NS_ERROR_FAILURE);
    BWindow *bwindow = (BWindow *)(mainWidget->GetNativeData(NS_NATIVE_WINDOW));
    if (bwindow)
        bwindow->Activate(true);
    return NS_OK;
}


class nsNativeAppSupportBeOS : public nsNativeAppSupportBase
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSINATIVEAPPSUPPORT
    static void HandleCommandLine( int32 argc, char **argv, PRUint32 aState);
}; 


class nsBeOSApp : public BApplication
{
public:
    nsBeOSApp(sem_id sem) : BApplication( GetAppSig() ), init(sem), mMessage(NULL)
    {}

  	~nsBeOSApp()
  	{
        delete mMessage;
  	}

    void ReadyToRun() 
    {
        release_sem(init);
    }

    static int32 Main( void *args ) 
    {
        nsBeOSApp *app = new nsBeOSApp((sem_id)args);
        if (app == NULL)
            return B_ERROR;
        return app->Run();
    }
    
    void ArgvReceived(int32 argc, char **argv)
    {
        if (IsLaunching())
        {
#ifdef DC_PROGRAMNAME
       		TRACE("ArgvReceived Launching\n");
#endif
       		return;
        }
        PRInt32 aState = 

                         nsICommandLine::STATE_REMOTE_AUTO;
        nsNativeAppSupportBeOS::HandleCommandLine(argc, argv, aState);
    }

    void RefsReceived(BMessage* msg)
    {
#ifdef DC_PROGRAMNAME
        TRACE("RefsReceived\n");
#endif
        if (IsLaunching())
        {
           mMessage = new BMessage(*msg);
           return;
        }
        BPath path;
        entry_ref er;
        for (uint32 i = 0; msg->FindRef("refs", i, &er) == B_OK; i++)
        {
            int Argc = 2;
            char **Argv = new char*[ 3 ];
            BEntry entry(&er, true);
            BEntry fentry(GetAppFile(), false);
            entry.GetPath(&path);
            
            Argv[0] = strdup( GetAppFile() ? GetAppFile() : "" );
            Argv[1] = strdup( path.Path() ? path.Path() : "" );
            
            Argv[2] = 0;
            
            
            ArgvReceived(2, Argv);
            Argc = 0;
            delete [] Argv;
            Argv = NULL;
        } 
    }
    
    void MessageReceived(BMessage* msg)
    {
        
        
        if (msg->what == 'enbl' && mMessage)
        {
#ifdef DC_PROGRAMNAME
            TRACE("enbl received");
#endif
            be_app_messenger.SendMessage(mMessage);
        }
        
        
        else if (msg->what == B_SIMPLE_DATA)
        {
            RefsReceived(msg);
        }
        else
            BApplication::MessageReceived(msg);
    }
private:
    char *GetAppSig()
    {
        image_info info;
        int32 cookie = 0;
        BFile file;
        BAppFileInfo appFileInfo;
        static char sig[B_MIME_TYPE_LENGTH];

        sig[0] = 0;
        if (get_next_image_info(0, &cookie, &info) == B_OK &&
            file.SetTo(info.name, B_READ_ONLY) == B_OK &&
            appFileInfo.SetTo(&file) == B_OK &&
            appFileInfo.GetSignature(sig) == B_OK)
            return sig;

        return "application/x-vnd.Mozilla";
    }
    
    char *GetAppFile()
    {
        image_info info;
        int32 cookie = 0;
        if (get_next_image_info(0, &cookie, &info) == B_OK && strlen(info.name) > 0)
            return info.name;
        
        return "";
    }

    sem_id init;
    BMessage *mMessage;
}; 


nsresult
NS_CreateNativeAppSupport(nsINativeAppSupport **aResult)
{
    if (!aResult)
        return NS_ERROR_NULL_POINTER;

    nsNativeAppSupportBeOS *pNative = new nsNativeAppSupportBeOS;
    if (!pNative)
        return NS_ERROR_OUT_OF_MEMORY;

    *aResult = pNative;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsNativeAppSupportBeOS, nsINativeAppSupport)


void
nsNativeAppSupportBeOS::HandleCommandLine(int32 argc, char **argv, PRUint32 aState)
{
    nsresult rv;
    
    
    
    
    nsCOMPtr<nsICommandLineRunner> cmdLine(do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
    if (!cmdLine)
    {
#ifdef DC_PROGRAMNAME
        TRACE("Couldn't create command line!");
#endif
        return;
    }
   
    
    
    nsCOMPtr<nsICommandLineRunner> cmdLineProxy;
    rv = NS_GetProxyForObject(  NS_PROXY_TO_MAIN_THREAD, NS_GET_IID(nsICommandLineRunner), 
        cmdLine, NS_PROXY_ASYNC | NS_PROXY_ALWAYS, getter_AddRefs(cmdLineProxy));
    if (rv != NS_OK)
    {
#ifdef DC_PROGRAMNAME
        TRACE("Couldn't get command line Proxy!");
#endif
        return;
    }
    
    
    
    rv = cmdLine->Init(argc, argv, 0 , aState);
    if (rv != NS_OK)
    {
#ifdef DC_PROGRAMNAME
        TRACE("Couldn't init command line!");
#endif
        return;
    }

    nsCOMPtr<nsIDOMWindowInternal> navWin;
    GetMostRecentWindow( NS_LITERAL_STRING( "navigator:browser" ).get(),
                         getter_AddRefs(navWin ));
    if (navWin)
    {
# ifdef DC_PROGRAMNAME
        TRACE("GotNavWin!");
# endif
        cmdLine->SetWindowContext(navWin);
    }





    cmdLineProxy->Run();
}

NS_IMETHODIMP
nsNativeAppSupportBeOS::Start(PRBool *aResult) 
{
    NS_ENSURE_ARG(aResult);
    NS_ENSURE_TRUE(be_app == NULL, NS_ERROR_NOT_INITIALIZED);
    sem_id initsem = create_sem(0, "Mozilla BApplication init");
    if (initsem < B_OK)
        return NS_ERROR_FAILURE;
    thread_id tid = spawn_thread(nsBeOSApp::Main, "Mozilla XUL BApplication", B_NORMAL_PRIORITY, (void *)initsem);
#ifdef DC_PROGRAMNAME
    TRACE("BeApp created");
#endif
    *aResult = PR_TRUE;
    if (tid < B_OK || B_OK != resume_thread(tid))
        *aResult = PR_FALSE;

    if (B_OK != acquire_sem(initsem))
        *aResult = PR_FALSE;
    
    if (B_OK != delete_sem(initsem))
        *aResult = PR_FALSE;
    return *aResult == PR_TRUE ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNativeAppSupportBeOS::Stop(PRBool *aResult) 
{
    NS_ENSURE_ARG(aResult);
    NS_ENSURE_TRUE(be_app, NS_ERROR_NOT_INITIALIZED);

    *aResult = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBeOS::Quit() 
{
    if (be_app->Lock())
    {
        be_app->Quit();
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNativeAppSupportBeOS::ReOpen()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsNativeAppSupportBeOS::Enable()
{
    
    if (be_app)
    {
        be_app_messenger.SendMessage('enbl');
    }
    return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportBeOS::OnLastWindowClosing()
{
    return NS_OK;
}
