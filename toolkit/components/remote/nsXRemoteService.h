






#ifndef NSXREMOTESERVICE_H
#define NSXREMOTESERVICE_H

#include "nsString.h"

#include "nsIRemoteService.h"
#include "nsIObserver.h"
#include <X11/Xlib.h>
#include <X11/X.h>

class nsIDOMWindow;
class nsIWeakReference;




class nsXRemoteService : public nsIRemoteService,
                         public nsIObserver
{
public:
    NS_DECL_NSIOBSERVER


protected:
    nsXRemoteService();

    static bool HandleNewProperty(Window aWindowId,Display* aDisplay,
                                    Time aEventTime, Atom aChangedAtom,
                                    nsIWeakReference* aDomWindow);
    
    void XRemoteBaseStartup(const char *aAppName, const char *aProfileName);

    void HandleCommandsFor(Window aWindowId);
    static nsXRemoteService *sRemoteImplementation;
private:
    void EnsureAtoms();
    static const char* HandleCommand(char* aCommand, nsIDOMWindow* aWindow,
                                     uint32_t aTimestamp);

    static const char* HandleCommandLine(char* aBuffer, nsIDOMWindow* aWindow,
                                         uint32_t aTimestamp);

    virtual void SetDesktopStartupIDOrTimestamp(const nsACString& aDesktopStartupID,
                                                uint32_t aTimestamp) = 0;

    nsCString mAppName;
    nsCString mProfileName;

    static Atom sMozVersionAtom;
    static Atom sMozLockAtom;
    static Atom sMozCommandAtom;
    static Atom sMozResponseAtom;
    static Atom sMozUserAtom;
    static Atom sMozProfileAtom;
    static Atom sMozProgramAtom;
    static Atom sMozCommandLineAtom;
};

#endif 
