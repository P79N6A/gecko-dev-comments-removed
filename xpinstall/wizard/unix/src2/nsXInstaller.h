






































#ifndef _NS_XINSTALLER_H_
#define _NS_XINSTALLER_H_

#include "XIDefines.h"
#include "XIErrors.h"

#include "nsINIParser.h"
#include "nsLicenseDlg.h"
#include "nsXIContext.h"

extern nsXIContext *gCtx; 

class nsXInstaller
{
public:
    nsXInstaller();
    ~nsXInstaller();

    int ParseArgs(int aArgc, char **aArgv);
    int ParseConfig();
    int RunWizard(int argc, char **argv);
    int ParseGeneral(nsINIParser *aParser);

    static gint Kill(GtkWidget *widget, GtkWidget *event, gpointer data);

private:
    int         InitContext();
    GtkWidget   *DrawLogo();
    int         DrawCancelButton(GtkWidget *aLogoVBox);
    int         DrawNavButtons();
};

int     main(int argc, char **argv);
int     ErrorHandler(int aErr, const char *aErrMsg=NULL);
void    ErrDlgOK(GtkWidget *aWidget, gpointer aData);
int     IsErrFatal(int aErr);

#define CONFIG "config"

#if defined(DUMP)
#undef DUMP
#endif
#if defined(DEBUG_sgehani) || defined(DEBUG_druidd) || defined(DEBUG_root)
#define DUMP(_msg) printf("%s %d: %s \n", __FILE__, __LINE__, _msg);
#else
#define DUMP(_msg)
#endif


#endif 
