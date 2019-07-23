






































#ifndef _NS_WELCOMEDLG_H_
#define _NS_WELCOMEDLG_H_

#include "nsXInstallerDlg.h"
#include "XIErrors.h"

class nsWelcomeDlg : public nsXInstallerDlg
{
public:
    nsWelcomeDlg();
    ~nsWelcomeDlg();




    static void     Next(GtkWidget *aWidget, gpointer aData);
    
    int     Parse(nsINIParser *aParser);

    int     Show();
    int     Hide();




    int     SetReadmeFile(char *aReadmeFile);
    char    *GetReadmeFile();
    char    *GetReadmeContents();
    
private:
    char    *mReadmeFile;
};

#endif 
