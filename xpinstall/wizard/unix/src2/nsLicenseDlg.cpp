






































#include "nsLicenseDlg.h"
#include "nsXInstaller.h"
#include <sys/stat.h>

nsLicenseDlg::nsLicenseDlg() :
    mLicenseFile(NULL)
{
}

nsLicenseDlg::~nsLicenseDlg()
{
    XI_IF_FREE(mLicenseFile);
}

void
nsLicenseDlg::Back(GtkWidget *aWidget, gpointer aData)
{
    DUMP("Back");
    if (aData != gCtx->ldlg) return;
#ifdef MOZ_WIDGET_GTK
    if (gCtx->bMoving) 
    {
        gCtx->bMoving = FALSE;
        return;
    }
#endif
    
    gtk_main_quit();
    return;
}

void
nsLicenseDlg::Next(GtkWidget *aWidget, gpointer aData)
{
    DUMP("Next");
    if (aData != gCtx->ldlg) return;
#ifdef MOZ_WIDGET_GTK
    if (gCtx->bMoving) 
    {
        gCtx->bMoving = FALSE;
        return;
    }
#endif

    
    gCtx->ldlg->Hide();

    
    gCtx->sdlg->Show();
#ifdef MOZ_WIDGET_GTK
    gCtx->bMoving = TRUE;
#endif
}

int
nsLicenseDlg::Parse(nsINIParser *aParser)
{
    int err = OK;
    int bufsize = 0;
    char *showDlg = NULL;
    
    
    XI_ERR_BAIL(aParser->GetStringAlloc(DLG_LICENSE, LICENSE, 
                &mLicenseFile, &bufsize));
    if (bufsize == 0 || !mLicenseFile)
        return E_INVALID_KEY;

    
    bufsize = 5;
    XI_ERR_BAIL(aParser->GetStringAlloc(DLG_LICENSE, SHOW_DLG, &showDlg,
                                        &bufsize));
    if (bufsize != 0 && showDlg)
    {
        if (0 == strncmp(showDlg, "TRUE", 4))
            mShowDlg = nsXInstallerDlg::SHOW_DIALOG;
        else if (0 == strncmp(showDlg, "FALSE", 5))
            mShowDlg = nsXInstallerDlg::SKIP_DIALOG;
    }

    bufsize = 0;
    XI_ERR_BAIL(aParser->GetStringAlloc(DLG_LICENSE, TITLE, &mTitle,
                                        &bufsize));
    if (bufsize == 0)
            XI_IF_FREE(mTitle); 

    return err;

BAIL:
    return err;
}

int
nsLicenseDlg::Show()
{
    int err = OK;
    if (!mShowDlg)
    {
       gCtx->sdlg->Show();
       return err;
    }
    
    char *licenseContents = NULL;

    XI_VERIFY(gCtx);
    XI_VERIFY(gCtx->notebook);

    if (mWidgetsInit == FALSE) 
    {
        
        mTable = gtk_table_new(1, 3, FALSE);
        gtk_notebook_append_page(GTK_NOTEBOOK(gCtx->notebook), mTable, NULL);
        mPageNum = gtk_notebook_get_current_page(GTK_NOTEBOOK(gCtx->notebook));
        
        gtk_table_set_col_spacing(GTK_TABLE(mTable), 1, 0);
        gtk_widget_show(mTable);

        
        licenseContents = GetLicenseContents();
        if (!licenseContents)
        {
            err = ErrorHandler(E_EMPTY_LICENSE);
            goto BAIL;
        }

#if defined(MOZ_WIDGET_GTK)
        
        GtkWidget *text = gtk_text_new(NULL, NULL);
        GdkFont *font = gdk_font_load( LICENSE_FONT );
        gtk_text_set_editable(GTK_TEXT(text), FALSE);
        gtk_text_set_word_wrap(GTK_TEXT(text), TRUE);
        gtk_text_set_line_wrap(GTK_TEXT(text), TRUE);
        gtk_table_attach(GTK_TABLE(mTable), text, 1, 2, 0, 1,
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
			0, 0);
        gtk_text_freeze(GTK_TEXT(text));
        gtk_text_insert (GTK_TEXT(text), font, &text->style->black, NULL,
                          licenseContents, -1);
        gtk_text_thaw(GTK_TEXT(text));
        gtk_widget_show(text);

        
        GtkWidget *vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
        gtk_table_attach(GTK_TABLE(mTable), vscrollbar, 2, 3, 0, 1,
            GTK_FILL,
			static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_SHRINK | GTK_FILL),
			0, 0);
        gtk_widget_show(vscrollbar);
#elif defined(MOZ_WIDGET_GTK2)
        GtkWidget *text = gtk_scrolled_window_new (NULL, NULL);
        GtkWidget *textview = gtk_text_view_new();
        GtkTextBuffer *textbuffer;

        textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
        gtk_text_buffer_set_text (textbuffer, licenseContents, -1);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (text),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (text), textview);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(textview), FALSE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
        gtk_table_attach(GTK_TABLE(mTable), text, 1, 2, 0, 1,
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND ),
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND), 0, 0);
        gtk_widget_show_all(text);
#endif

        mWidgetsInit = TRUE;
    }
    else
    {
        gtk_notebook_set_page(GTK_NOTEBOOK(gCtx->notebook), mPageNum);
        gtk_widget_show(mTable);
    }

    
    gCtx->backID = gtk_signal_connect(GTK_OBJECT(gCtx->back), "clicked",
                   GTK_SIGNAL_FUNC(nsLicenseDlg::Back), gCtx->ldlg);
    gCtx->nextID = gtk_signal_connect(GTK_OBJECT(gCtx->next), "clicked",
                   GTK_SIGNAL_FUNC(nsLicenseDlg::Next), gCtx->ldlg);

    GTK_WIDGET_SET_FLAGS(gCtx->next, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(gCtx->next);
    gtk_widget_grab_focus(gCtx->next);
    
    
    gCtx->acceptLabel = gtk_label_new(gCtx->Res("ACCEPT"));
    gCtx->declineLabel = gtk_label_new(gCtx->Res("DECLINE"));
    gtk_container_add(GTK_CONTAINER(gCtx->next), gCtx->acceptLabel);
    gtk_container_add(GTK_CONTAINER(gCtx->back), gCtx->declineLabel);
    gtk_widget_show(gCtx->acceptLabel);
    gtk_widget_show(gCtx->declineLabel);
    gtk_widget_show(gCtx->next);
    gtk_widget_show(gCtx->back);

BAIL:
    XI_IF_FREE(licenseContents);
    return err;
}

int
nsLicenseDlg::Hide()
{
    
    gtk_widget_hide(mTable);

    
    gtk_signal_disconnect(GTK_OBJECT(gCtx->back), gCtx->backID);
    gtk_signal_disconnect(GTK_OBJECT(gCtx->next), gCtx->nextID);

    gtk_container_remove(GTK_CONTAINER(gCtx->back), gCtx->declineLabel);
    gtk_container_remove(GTK_CONTAINER(gCtx->next), gCtx->acceptLabel);

    gtk_widget_hide(gCtx->back);
    gtk_widget_hide(gCtx->next);
    
    return OK;
}

int
nsLicenseDlg::SetLicenseFile(char *aLicenseFile)
{
    if (!aLicenseFile)
        return E_PARAM;

    mLicenseFile = aLicenseFile;

    return OK;
}

char *
nsLicenseDlg::GetLicenseFile()
{
    if (mLicenseFile)
        return mLicenseFile;

    return NULL;
}

char *
nsLicenseDlg::GetLicenseContents()
{
    char *buf = NULL;
    FILE *fd = NULL;
    struct stat attr;
    int buflen;

    DUMP(mLicenseFile);
    if (!mLicenseFile)
        return NULL;
   
    
    fd = fopen(mLicenseFile, "r");
    if (!fd) return NULL;
    DUMP("license fopen");

    
    if (0 != stat(mLicenseFile, &attr)) return NULL;
    if (attr.st_size == 0) return NULL;
    DUMP("license fstat");

    
    buflen = sizeof(char) * (attr.st_size + 1);
    buf = (char *) malloc(buflen);
    if (!buf) return NULL;
    memset(buf, 0, buflen);
    DUMP("license buf malloc");

    
    if (attr.st_size != ((int) fread(buf, sizeof(char), attr.st_size, fd))) 
        XI_IF_FREE(buf);
    DUMP("license fread");

    
    fclose(fd);
    DUMP("license close");

    return buf;
}
