






































#ifndef _NS_COMPONENTSDLG_H_
#define _NS_COMPONENTSDLG_H_

#include "XIErrors.h"

#include "nsXInstallerDlg.h"
#include "nsComponentList.h"
#include "nsSetupType.h"

class nsComponentsDlg : public nsXInstallerDlg
{
public:
    nsComponentsDlg();
    ~nsComponentsDlg();




    static void     Back(GtkWidget *aWidget, gpointer aData);
    static void     Next(GtkWidget *aWidget, gpointer aData);

    int             Parse(nsINIParser* aParser);

    int             Show();
    int             Hide();
    
    static void     RowSelected(GtkWidget *aWidget, gint aRow, gint aColumn,
                               GdkEventButton *aEvent, gpointer aData);
    static gboolean KeyPressed(GtkWidget *aWidget, GdkEventKey *aEvent, 
                               gpointer aData);
    static void     ToggleRowSelection(GtkWidget *aEvent, gint aRow, 
                               gint aColumn);




    int             SetMsg0(char *aMsg);
    char            *GetMsg0();

    int             SetCompList(nsComponentList *aCompList);
    nsComponentList *GetCompList();

private:
    int             ParseDependees(char *aCurrSec, nsComponent *aCurrComp, 
                                   nsINIParser *aParser);
    char            *mMsg0;
    nsComponentList *mCompList;
};

#endif 
