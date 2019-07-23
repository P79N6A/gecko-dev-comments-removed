








































#ifndef __nsGTKRemoteService_h__
#define __nsGTKRemoteService_h__

#include "nsIRemoteService.h"

#include "nsIObserver.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "nsString.h"
#include "nsInterfaceHashtable.h"

class nsIDOMWindow;
class nsIWeakReference;
class nsIWidget;

class nsGTKRemoteService : public nsIRemoteService,
                           public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREMOTESERVICE
  NS_DECL_NSIOBSERVER

  nsGTKRemoteService() :
    mServerWindow(NULL) { }

private:
  ~nsGTKRemoteService() { }

  void HandleCommandsFor(GtkWidget* aWidget,
                         nsIWeakReference* aWindow);

  static void EnsureAtoms();
  static PLDHashOperator StartupHandler(const void* aKey,
                                        nsIWeakReference* aData,
                                        void* aClosure);

  static const char* HandleCommand(char* aCommand, nsIDOMWindow* aWindow,
                                   PRUint32 aTimestamp);

#ifdef MOZ_XUL_APP
  static const char* HandleCommandLine(char* aBuffer, nsIDOMWindow* aWindow,
                                       PRUint32 aTimestamp);
#endif

  static gboolean HandlePropertyChange(GtkWidget *widget,
                                       GdkEventProperty *event,
                                       nsIWeakReference* aThis);

  GtkWidget* mServerWindow;
  nsCString mAppName;
  nsCString mProfileName;
  nsInterfaceHashtable<nsVoidPtrHashKey, nsIWeakReference> mWindows;

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
