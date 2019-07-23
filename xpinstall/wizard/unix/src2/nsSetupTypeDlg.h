






































#ifndef _NS_SETUPTYPEDLG_H_
#define _NS_SETUPTYPEDLG_H_

#include <sys/stat.h>
#include "nsXInstallerDlg.h"
#include "nsSetupType.h"
#include "nsObjectIgnore.h"
#include "nsLegacyCheck.h"

class nsSetupTypeDlg : public nsXInstallerDlg
{
public:
    nsSetupTypeDlg();
    ~nsSetupTypeDlg();




    static void         Next(GtkWidget *aWidget, gpointer aData);

    int                 Parse(nsINIParser *aParser);

    int                 Show();
    int                 Hide();

    static void         SelectFolder(GtkWidget *aWidget, gpointer aData);
    static void         SelectFolderOK(GtkWidget *aWidget, 
                                       GtkFileSelection *aFileSel);
    static void         SelectFolderCancel(GtkWidget *aWidget, 
                                       GtkFileSelection *aFileSel);
    static void         RadBtnToggled(GtkWidget *aWidget, gpointer aData);
    static int          VerifyDestination();
    static void         NoPermsOK(GtkWidget *aWidget, gpointer aData);
    static void         CreateDestYes(GtkWidget *aWidget, gpointer aData);
    static void         CreateDestNo(GtkWidget *aWidget, gpointer aData);
    static int          DeleteOldInst();
    static void         DeleteInstDelete(GtkWidget *aWidget, gpointer aData);
    static void         DeleteInstCancel(GtkWidget *aWidget, gpointer aData);
    static int          ConstructPath(char *aDest, char *aTrunk, char *aLeaf);
    static int          CheckDestEmpty();
    static int          VerifyDiskSpace();
    static int          DSAvailable();
    static int          DSRequired();
    static void         NoDiskSpaceOK(GtkWidget *aWidget, gpointer aData);




    int                 SetMsg0(char *aMsg);
    char                *GetMsg0();

    int                 AddSetupType(nsSetupType *aSetupType);
    nsSetupType         *GetSetupTypeList();
    int                 GetNumSetupTypes();
    nsSetupType         *GetSelectedSetupType();

private:
    void                FreeSetupTypeList();
    void                FreeLegacyChecks();

    char                    *mMsg0;
    nsSetupType             *mSetupTypeList;
};

#endif 
