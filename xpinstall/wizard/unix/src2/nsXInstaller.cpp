






































#include "nscore.h"
#include "nsXInstaller.h"
#include "logo.xpm"

nsXIContext *gCtx = NULL;
static GtkWidget *sErrDlg = NULL;

nsXInstaller::nsXInstaller()
{
}

nsXInstaller::~nsXInstaller()
{
    XI_IF_DELETE(gCtx);
}

int 
nsXInstaller::ParseArgs(int aArgc, char **aArgv)
{
    if (aArgc <= 0 || !aArgv)
        return E_PARAM;

    for (int argNum = 1; argNum < aArgc; ++argNum)
    {
        

        if (strcmp(aArgv[argNum], "-h") == 0 ||
            strcmp(aArgv[argNum], "--help") == 0)
        {
           if (gCtx->Res("USAGE_MSG"))
                fprintf (stderr, gCtx->Res("USAGE_MSG"), aArgv[0], "\n", 
                      "\n", "\n", "\n", "\n", "\n", "\n", "\n");
            return E_USAGE_SHOWN;
        }

        


        else if (strcmp(aArgv[argNum], "-ma") == 0)
        {
            gCtx->opt->mMode = nsXIOptions::MODE_AUTO;
        }
        
        


        else if (strcmp(aArgv[argNum], "-ms") == 0)
        {
            gCtx->opt->mMode = nsXIOptions::MODE_SILENT;
        }

        

        else if (strcmp(aArgv[argNum], "-ira") == 0)
        {
            gCtx->opt->mShouldRunApps = FALSE;
        }
    }

    return OK;
}

int
nsXInstaller::ParseConfig()
{
    int err = OK;
    nsINIParser *parser = NULL; 
    char *cfg = NULL;

    XI_ERR_BAIL(InitContext());
    err = gCtx->LoadResources();
    if (err != OK)
        return err;

    cfg = nsINIParser::ResolveName(CONFIG);
    if (!cfg)
        return E_INVALID_PTR;

    parser = new nsINIParser(cfg);
    if (!parser)
    {
        err = E_MEM;    
        goto BAIL;
    }

    err = parser->GetError();
    if (err != nsINIParser::OK)
        return err;

    XI_ERR_BAIL(ParseGeneral(parser));
    XI_ERR_BAIL(gCtx->ldlg->Parse(parser));
    XI_ERR_BAIL(gCtx->wdlg->Parse(parser));
    XI_ERR_BAIL(gCtx->cdlg->Parse(parser)); 
    XI_ERR_BAIL(gCtx->sdlg->Parse(parser));
    XI_ERR_BAIL(gCtx->idlg->Parse(parser));

BAIL:
    XI_IF_FREE(cfg);
    if (err != OK)
        ErrorHandler(err);
    return err;
}

int 
nsXInstaller::InitContext()
{
    int err = OK;

    gCtx = new nsXIContext();
    if (!gCtx)
        return E_MEM;

    gCtx->me = this;

    gCtx->ldlg = new nsLicenseDlg();
    gCtx->wdlg = new nsWelcomeDlg();
    gCtx->sdlg = new nsSetupTypeDlg();
    gCtx->cdlg = new nsComponentsDlg();
    gCtx->idlg = new nsInstallDlg();
    if (!gCtx->ldlg || !gCtx->wdlg || !gCtx->sdlg || 
        !gCtx->cdlg || !gCtx->idlg )
    {
        err = E_MEM;
        goto BAIL;
    }

    return OK;
    
BAIL:
    XI_IF_DELETE(gCtx->ldlg);
    XI_IF_DELETE(gCtx->wdlg);
    XI_IF_DELETE(gCtx->sdlg);
    XI_IF_DELETE(gCtx->cdlg);
    XI_IF_DELETE(gCtx->idlg);
    XI_IF_DELETE(gCtx);

    return err;
}

int 
nsXInstaller::RunWizard(int argc, char **argv)
{
    int err = OK;
    GtkWidget *logovbox;

    XI_VERIFY(gCtx);

    
    if (gCtx->opt->mMode != nsXIOptions::MODE_SILENT) {
        gtk_set_locale();
        gtk_init(&argc, &argv);
        gdk_rgb_init();

        gCtx->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        XI_VERIFY(gCtx->window);

        gtk_window_set_position(GTK_WINDOW(gCtx->window), GTK_WIN_POS_CENTER);
        gtk_signal_connect(GTK_OBJECT(gCtx->window), "delete_event",
                           GTK_SIGNAL_FUNC(Kill), NULL);

        gtk_widget_set_usize(gCtx->window, XI_WIN_WIDTH, XI_WIN_HEIGHT);
        gtk_container_set_border_width(GTK_CONTAINER(gCtx->window), 5);
        gtk_window_set_title(GTK_WINDOW(gCtx->window), gCtx->opt->mTitle);
        gtk_widget_show(gCtx->window);

        
        logovbox = DrawLogo();
        if (logovbox)
            DrawCancelButton(logovbox);

        
        XI_ERR_BAIL(DrawNavButtons());

        
        gCtx->notebook = gtk_notebook_new();
        XI_VERIFY(gCtx->notebook);
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gCtx->notebook), FALSE);
        gtk_notebook_set_show_border(GTK_NOTEBOOK(gCtx->notebook), FALSE);
        gtk_notebook_set_scrollable(GTK_NOTEBOOK(gCtx->notebook), FALSE);
        gtk_widget_show(gCtx->notebook);
        gtk_container_add(GTK_CONTAINER(gCtx->canvas), gCtx->notebook);
    }

    if (gCtx->opt->mMode == nsXIOptions::MODE_DEFAULT)
    {
        
        gCtx->wdlg->Show(); 

        
        gtk_main();

    }
    else
    {
        
        gCtx->sdlg->Next((GtkWidget *)NULL, (gpointer) gCtx->sdlg);
    }

    return OK;

BAIL:
    return err;
}

gint
nsXInstaller::Kill(GtkWidget *widget, GtkWidget *event, gpointer data)
{
    gtk_main_quit();
    return FALSE;
}

GtkWidget *
nsXInstaller::DrawLogo()
{
    GtkWidget *logo = NULL;
    GdkPixmap *pixmap = NULL;
    GdkBitmap *mask = NULL;
    GtkStyle *style = NULL;
    GtkWidget *mainhbox = NULL;
    GtkWidget *logovbox = NULL;
    GtkWidget *canvasvbox = NULL;

    style = gtk_widget_get_style(gCtx->window);
    pixmap = gdk_pixmap_create_from_xpm_d(gCtx->window->window, &mask,
                                          &style->bg[GTK_STATE_NORMAL],
                                          (gchar **)logo_xpm);

    logo = gtk_pixmap_new(pixmap, mask);
    gtk_widget_show(logo);

    mainhbox = gtk_hbox_new(FALSE, 10);
    logovbox = gtk_vbox_new(FALSE, 30);
    canvasvbox = gtk_vbox_new(FALSE, 10);

    gtk_box_pack_start(GTK_BOX(logovbox), logo, FALSE, FALSE, 0);
    gtk_widget_show(logovbox);
    gtk_widget_show(canvasvbox);

    gtk_box_pack_start(GTK_BOX(mainhbox), logovbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mainhbox), canvasvbox, TRUE, TRUE, 0);
    gtk_widget_show(mainhbox);

    gtk_container_add(GTK_CONTAINER(gCtx->window), mainhbox);

    gCtx->mainbox = canvasvbox; 

    return logovbox;
}

int
nsXInstaller::DrawCancelButton(GtkWidget *aLogoVBox)
{
    int err = OK;
    GtkWidget *hbox;

    gCtx->cancel = gtk_button_new_with_label(gCtx->Res("CANCEL"));
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), gCtx->cancel, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(aLogoVBox), hbox, FALSE, TRUE, 10);
    gCtx->cancelID = gtk_signal_connect(GTK_OBJECT(gCtx->cancel), "clicked",
                        GTK_SIGNAL_FUNC(Kill), NULL);
    gtk_widget_show(hbox);
    gtk_widget_show(gCtx->cancel);

    return err;
}

int
nsXInstaller::DrawNavButtons()
{
    int err = OK;
    
    GtkWidget *navbtnhbox;
    GtkWidget *canvasvbox;
    GtkWidget *navbtntable;

    XI_VERIFY(gCtx->mainbox);

    gCtx->next = gtk_button_new();  
    gCtx->back = gtk_button_new(); 
    XI_VERIFY(gCtx->next);
    XI_VERIFY(gCtx->back);
    
    navbtnhbox = gtk_hbox_new(TRUE, 10);
    canvasvbox = gtk_vbox_new(TRUE, 10);
    gtk_box_pack_start(GTK_BOX(gCtx->mainbox), canvasvbox, TRUE, TRUE, 0); 
    gtk_box_pack_start(GTK_BOX(gCtx->mainbox), navbtnhbox, FALSE, FALSE, 0); 

    
    navbtntable = gtk_table_new(1, 4, TRUE);
    gtk_box_pack_start(GTK_BOX(navbtnhbox), navbtntable, TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(navbtntable), gCtx->back, 2, 3, 0, 1, 
        static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
               static_cast<GtkAttachOptions>(GTK_SHRINK),
               5, 5);
    gtk_table_attach(GTK_TABLE(navbtntable), gCtx->next, 3, 4, 0, 1,
        static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
               static_cast<GtkAttachOptions>(GTK_SHRINK),
               5, 5);

    gtk_widget_show(navbtntable); 
    gtk_widget_show(navbtnhbox); 
    gtk_widget_show(canvasvbox);

    GTK_WIDGET_SET_FLAGS(gCtx->next, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(gCtx->next);
    gtk_widget_grab_focus(gCtx->next);
    gtk_widget_show(gCtx->mainbox);

    XI_VERIFY(canvasvbox);
    gCtx->canvas = canvasvbox; 

    return err;
}

int
nsXInstaller::ParseGeneral(nsINIParser *aParser)
{
    int     err = OK;
    char    *dest = NULL, *title = NULL;
    int     size = 0;
 
    
    err = aParser->GetStringAlloc(GENERAL, DEFAULT_LOCATION, &dest, &size);
    if (err == OK && size > 0)
    {
        


        gCtx->opt->mDestination = (char *)malloc(MAXPATHLEN * sizeof(char));
        strncpy(gCtx->opt->mDestination, dest, size);
        XI_IF_FREE(dest);
    }
    else
        err = OK; 

    
    size = 0;
    err = aParser->GetStringAlloc(GENERAL, TITLE, &title, &size);
    if (err == OK && size > 0)
        gCtx->opt->mTitle = title;
    else
    {
        err = OK; 
        gCtx->opt->mTitle = strdup(gCtx->Res("DEFAULT_TITLE"));
    }

    return err;
}

int
main(int argc, char **argv)
{
    nsXInstaller *installer = new nsXInstaller();
    int err = OK;

    if (installer)
    {
        if ( (err = installer->ParseConfig()) == OK)
        {
            if (installer->ParseArgs(argc, argv) == OK)
                err = installer->RunWizard(argc, argv);
        }
    }
    else
        err = E_MEM;

    XI_IF_DELETE(installer);

    exit(err);
}




int 
ErrorHandler(int aErr, const char* aErrMsg)
{
    GtkWidget *okButton, *label;
    char msg[256];
    char newmsg[256];
    char errStr[16];
    
    sprintf(errStr, "%d", aErr); 
    if (aErrMsg != NULL)
       sprintf(newmsg, gCtx->Res(errStr), aErrMsg);
    else
       strcpy(newmsg, gCtx->Res(errStr));

    if (!IsErrFatal(aErr))
        sprintf(msg, gCtx->Res("ERROR"), aErr, newmsg);
    else
        sprintf(msg, gCtx->Res("FATAL_ERROR"), aErr, newmsg);
    
    
    
    if (gCtx->opt->mMode == nsXIOptions::MODE_SILENT || !gCtx->window)
    {
        fprintf (stderr, "%s\n", msg);
        if (IsErrFatal(aErr))
            exit(aErr);
        return aErr;
    }
    sErrDlg = gtk_dialog_new();
    gtk_window_set_modal(GTK_WINDOW(sErrDlg), TRUE);
    gtk_window_set_title(GTK_WINDOW(sErrDlg), gCtx->Res("ERROR_TITLE"));
    okButton = gtk_button_new_with_label(gCtx->Res("OK_LABEL"));
    label = gtk_label_new(msg);

    gtk_window_set_position(GTK_WINDOW(sErrDlg), GTK_WIN_POS_CENTER);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sErrDlg)->action_area), 
                      okButton);
    gtk_signal_connect(GTK_OBJECT(okButton), "clicked",
                       GTK_SIGNAL_FUNC(ErrDlgOK), NULL);
    gtk_signal_connect(GTK_OBJECT(sErrDlg), "delete_event",
                       GTK_SIGNAL_FUNC(ErrDlgOK), NULL);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sErrDlg)->vbox), label);
    
    GTK_WIDGET_SET_FLAGS(okButton, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(okButton);
    gtk_widget_show_all(sErrDlg);

    gtk_main();

    if (IsErrFatal(aErr))
        exit(aErr);

    return aErr;
}

void
ErrDlgOK(GtkWidget *aWidget, gpointer aData)
{
    gtk_widget_destroy(sErrDlg);
    sErrDlg = NULL;

    gtk_main_quit();
}

int
IsErrFatal(int aErr)
{
    int bFatal = TRUE;

    
    switch (aErr)
    {
        case E_XPI_FAIL:
        case E_INSTALL:
        case E_MKDIR_FAIL:
        case E_OLD_INST:
        case E_DIR_NOT_EMPTY:
        case E_INVALID_PROXY:
            bFatal = FALSE;
        default:
            break; 
    }

    return bFatal;
}
