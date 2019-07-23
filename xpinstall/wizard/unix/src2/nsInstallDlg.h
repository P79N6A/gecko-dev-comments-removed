






































#ifndef _NS_INSTALLDLG_H_
#define _NS_INSTALLDLG_H_

#include "nsXInstallerDlg.h"
#include "XIErrors.h"
#include "nsRunApp.h"

class nsComponent;

class nsInstallDlg : public nsXInstallerDlg
{
public:
    nsInstallDlg();
    ~nsInstallDlg();




    static void     Back(GtkWidget *aWidget, gpointer aData);
    static void     Next(GtkWidget *aWidget, gpointer aData);

    int             Parse(nsINIParser* aParser);

    int             Show();
    int             Hide();
    int             ShowTable();
    int             HideTable();

    static void     XPIProgressCB(const char *aMsg, int aVal, int aMax);
    static void     MajorProgressCB(char *aName, int aNum, int aTotal, 
                                    int aActivity);
    static int      DownloadCB(int aBytesRd, int aTotal);
    static void     SetDownloadComp(nsComponent *aComp, int aURLIndex,
                                    int aNum, int aTotal);
    static void     ClearRateLabel();
    static int      CancelOrPause();

    void            ReInitUI();
    void            ShowCRCDlg();
    void            DestroyCRCDlg();

    enum
    {
        ACT_DOWNLOAD,
        ACT_EXTRACT,
        ACT_INSTALL,
        ACT_COMPLETE
    };

    enum
    {
        E_DL_PAUSE      = -1101,
        E_DL_CANCEL     = -1102,
        E_DL_DROP_CXN   = -1103
    };




    int             SetMsg0(char *aMsg);
    char            *GetMsg0();

private:
    static int      PerformInstall(void); 
    static void     SaveModulesToggled(GtkWidget *aWidget, gpointer aData);
    static void     ShowProxySettings(GtkWidget *aWidget, gpointer aData);
    static void     PSDlgOK    (GtkWidget *aWidget, gpointer aData);
    static void     PSDlgCancel(GtkWidget *aWidget, gpointer aData);
    static void     RunApps(nsRunApp *aRunAppList, int aSequential);
    static void     FreeRunAppList(nsRunApp *aRunAppList);
    int             AppendRunApp(nsRunApp **aRunAppList, nsRunApp *aNewRunApp);
    static void     DLPause(GtkWidget *aWidget, gpointer aData);
    static void     DLResume(GtkWidget *aWidget, gpointer aData);
    static void     DLCancel(GtkWidget *aWidget, gpointer aData);
    static int      ShowCxnDroppedDlg();
    static void     CxnDroppedOK(GtkWidget *aWidget, gpointer aData);
    static void     CRCOKCb(GtkWidget *aWidget, gpointer aData);
    static void     HideNavButtons();
    static void     InitInstallProgress();
    static int      TotalDLSize();
    static void     CompressToFit(char *aOrigStr, char *aOutStr, 
                        int aOutStrLen);
    static void     InitDLProgress(int IsFirst);

    char            *mMsg0;
};

#endif 
