






































#ifndef _NS_XINSTALLERDLG_H_
#define _NS_XINSTALLERDLG_H_

#include <malloc.h>
#include "XIErrors.h"
#include "XIDefines.h"

#include "nsINIParser.h"







class nsXInstallerDlg
{
public:
    nsXInstallerDlg();
    virtual ~nsXInstallerDlg();




    
    
    
    

    virtual int     Parse(nsINIParser *aParser) = 0;

    virtual int     Show() = 0;
    virtual int     Hide() = 0;




    int             SetShowDlg(int aShowDlg);
    int             GetShowDlg();
    int             SetTitle(char *aTitle);
    char            *GetTitle();

    void            SetPageNum(int aPageNum);
    int             GetPageNum();

    enum
    {
        SKIP_DIALOG     = 0,
        SHOW_DIALOG     = 1
    };

protected:
    int         mShowDlg;
    char        *mTitle;
    int         mPageNum;      
    int         mWidgetsInit;  
    GtkWidget   *mTable;
};

#endif 
