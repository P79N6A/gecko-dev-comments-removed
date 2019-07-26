






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
    
    NS_DECL_ISUPPORTS
    
    NS_DECL_NSICLIPBOARD
    NS_DECL_NSIOBSERVER

    
    
    nsresult  Init              (void);

    
    void   SelectionGetEvent    (GtkClipboard     *aGtkClipboard,
                                 GtkSelectionData *aSelectionData);
    void   SelectionClearEvent  (GtkClipboard     *aGtkClipboard);

private:
    virtual ~nsClipboard();

    
    static GdkAtom               GetSelectionAtom (int32_t aWhichClipboard);
    static GtkSelectionData     *GetTargets       (GdkAtom aWhichClipboard);

    
    nsresult                     Store            (void);

    
    
    nsITransferable             *GetTransferable  (int32_t aWhichClipboard);

    
    
    nsCOMPtr<nsIClipboardOwner>  mSelectionOwner;
    nsCOMPtr<nsIClipboardOwner>  mGlobalOwner;
    nsCOMPtr<nsITransferable>    mSelectionTransferable;
    nsCOMPtr<nsITransferable>    mGlobalTransferable;
    nsRefPtr<nsClipboardPrivacyHandler> mPrivacyHandler;

};

#endif 
