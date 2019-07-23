





































#ifndef __nsClipboard_h_
#define __nsClipboard_h_

#include "nsIClipboard.h"
#include <gtk/gtkselection.h>

class nsClipboard : public nsIClipboard
{
public:
    nsClipboard();
    virtual ~nsClipboard();
    
    NS_DECL_ISUPPORTS
    
    NS_DECL_NSICLIPBOARD

    
    
    nsresult  Init                (void);
    
    void      SelectionGetEvent   (GtkWidget         *aWidget,
                                   GtkSelectionData  *aSelectionData,
                                   guint              aTime);
    void      SelectionClearEvent (GtkWidget         *aWidget,
                                   GdkEventSelection *aEvent);


private:
    
    static GdkAtom               GetSelectionAtom (PRInt32 aWhichClipboard);
    static GtkSelectionData     *GetTargets       (GdkAtom aWhichClipboard);

    
    
    nsITransferable             *GetTransferable  (PRInt32 aWhichClipboard);

    
    void                         AddTarget        (GdkAtom aName,
                                                   GdkAtom aClipboard);

    
    GtkWidget                   *mWidget;
    
    
    nsCOMPtr<nsIClipboardOwner>  mSelectionOwner;
    nsCOMPtr<nsIClipboardOwner>  mGlobalOwner;
    nsCOMPtr<nsITransferable>    mSelectionTransferable;
    nsCOMPtr<nsITransferable>    mGlobalTransferable;

};

#endif 
