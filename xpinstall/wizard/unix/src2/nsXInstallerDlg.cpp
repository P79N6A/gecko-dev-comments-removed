






































#include "nsXInstallerDlg.h"

nsXInstallerDlg::nsXInstallerDlg() :
    mShowDlg(nsXInstallerDlg::SHOW_DIALOG),
    mTitle(NULL),
    mWidgetsInit((int)FALSE),
    mTable(NULL)
{
}

nsXInstallerDlg::~nsXInstallerDlg()
{
    if (mTitle)
        free (mTitle);
}

int
nsXInstallerDlg::SetShowDlg(int aShowDlg)
{
    if ( aShowDlg != nsXInstallerDlg::SHOW_DIALOG &&
         aShowDlg != nsXInstallerDlg::SKIP_DIALOG )
        return E_PARAM;

    mShowDlg = aShowDlg;

    return OK;
}

int 
nsXInstallerDlg::GetShowDlg()
{
    return mShowDlg;
}

int
nsXInstallerDlg::SetTitle(char *aTitle)
{
    if (!aTitle)
        return E_PARAM;
    
    mTitle = aTitle;

    return OK;
}

char *
nsXInstallerDlg::GetTitle()
{
    if (mTitle)
        return mTitle;

    return NULL;
}
