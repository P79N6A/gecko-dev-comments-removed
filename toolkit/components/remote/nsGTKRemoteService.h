








































#ifndef __nsGTKRemoteService_h__
#define __nsGTKRemoteService_h__

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "nsInterfaceHashtable.h"
#include "nsXRemoteService.h"

class nsGTKRemoteService : public nsXRemoteService
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREMOTESERVICE


  nsGTKRemoteService() :
    mServerWindow(NULL) { }

private:
  ~nsGTKRemoteService() { }

  void HandleCommandsFor(GtkWidget* aWidget,
                         nsIWeakReference* aWindow);


  static PLDHashOperator StartupHandler(const void* aKey,
                                        nsIWeakReference* aData,
                                        void* aClosure);


  static gboolean HandlePropertyChange(GtkWidget *widget,
                                       GdkEventProperty *event,
                                       nsIWeakReference* aThis);


  virtual void SetDesktopStartupIDOrTimestamp(const nsACString& aDesktopStartupID,
                                              PRUint32 aTimestamp);

  nsInterfaceHashtable<nsVoidPtrHashKey, nsIWeakReference> mWindows;
  GtkWidget* mServerWindow;  
};

#endif 
