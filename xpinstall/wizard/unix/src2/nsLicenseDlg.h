






































#ifndef _NS_LICENSEDLG_H_
#define _NS_LICENSEDLG_H_

#include "nsXInstallerDlg.h"
#include "XIErrors.h"

class nsLicenseDlg : public nsXInstallerDlg
{
public:
    nsLicenseDlg();
    ~nsLicenseDlg();




    static void     Back(GtkWidget *aWidget, gpointer aData);
    static void     Next(GtkWidget *aWidget, gpointer aData);
    
    int     Parse(nsINIParser *aParser);

    int     Show();
    int     Hide();




    int     SetLicenseFile(char *aLicenseFile);
    char    *GetLicenseFile();
    char    *GetLicenseContents();
    
private:
    char    *mLicenseFile;
};

#endif 
