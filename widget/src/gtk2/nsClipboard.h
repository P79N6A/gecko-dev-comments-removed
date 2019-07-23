





































#ifndef __nsClipboard_h_
#define __nsClipboard_h_

#include "nsIClipboard.h"
#include "nsClipboardPrivacyHandler.h"
#include "nsAutoPtr.h"
#include <gtk/gtk.h>

class nsClipboard : public nsIClipboard,
                    public nsIObserver
{
public:
    nsClipboard();
    virtual ~nsClipboard();
    
    NS_DECL_ISUPPORTS
    
    NS_DECL_NSICLIPBOARD
    NS_DECL_NSIOBSERVER

    
    
    nsresult  Init              (void);

    
    void   SelectionGetEvent    (GtkClipboard     *aGtkClipboard,
                                 GtkSelectionData *aSelectionData);
    void   SelectionClearEvent  (GtkClipboard     *aGtkClipboard);

private:
    
    static GdkAtom               GetSelectionAtom (PRInt32 aWhichClipboard);
    static GtkSelectionData     *GetTargets       (GdkAtom aWhichClipboard);

    
    nsresult                     Store            (void);

    
    
    nsITransferable             *GetTransferable  (PRInt32 aWhichClipboard);

    
    
    nsCOMPtr<nsIClipboardOwner>  mSelectionOwner;
    nsCOMPtr<nsIClipboardOwner>  mGlobalOwner;
    nsCOMPtr<nsITransferable>    mSelectionTransferable;
    nsCOMPtr<nsITransferable>    mGlobalTransferable;
    nsRefPtr<nsClipboardPrivacyHandler> mPrivacyHandler;

};

#endif 
