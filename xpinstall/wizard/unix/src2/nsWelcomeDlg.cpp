






































#include "nsWelcomeDlg.h"
#include "nsXInstaller.h"
#include <sys/stat.h>


nsWelcomeDlg::nsWelcomeDlg() :
    mReadmeFile(NULL)
{
}

nsWelcomeDlg::~nsWelcomeDlg()
{
    if (mReadmeFile)
        free (mReadmeFile);
}

void
nsWelcomeDlg::Next(GtkWidget *aWidget, gpointer aData)
{
    DUMP("Next");
    if (aData != gCtx->wdlg)
        return;

    
    gCtx->wdlg->Hide();

    
    gCtx->ldlg->Show();
}

int
nsWelcomeDlg::Parse(nsINIParser *aParser)
{
    int err = OK;
    int bufsize = 0;
    char *showDlg = NULL;
    
    
    XI_ERR_BAIL(aParser->GetStringAlloc(DLG_WELCOME, README, &mReadmeFile,
                                        &bufsize));
    if (bufsize == 0 || !mReadmeFile)
        return E_INVALID_KEY;

    
    bufsize = 5;
    XI_ERR_BAIL(aParser->GetStringAlloc(DLG_WELCOME, SHOW_DLG, &showDlg,
                                        &bufsize));
    if (bufsize != 0 && showDlg)
    {
        if (0 == strncmp(showDlg, "TRUE", 4))
            mShowDlg = nsXInstallerDlg::SHOW_DIALOG;
        else if (0 == strncmp(showDlg, "FALSE", 5))
            mShowDlg = nsXInstallerDlg::SKIP_DIALOG;
    }

    bufsize = 0;
    XI_ERR_BAIL(aParser->GetStringAlloc(DLG_WELCOME, TITLE, &mTitle,
                                        &bufsize));
    if (bufsize == 0)
            XI_IF_FREE(mTitle); 

    return err;

BAIL:
    return err;
}

int
nsWelcomeDlg::Show()
{
    int err = OK;
    if (!mShowDlg)
    {
       gCtx->ldlg->Show();
       return err;
    }
   
    char *readmeContents = NULL;

    XI_VERIFY(gCtx);
    XI_VERIFY(gCtx->notebook);

    if (mWidgetsInit == FALSE) 
    {
        
        mTable = gtk_table_new(1, 3, FALSE);
        gtk_notebook_append_page(GTK_NOTEBOOK(gCtx->notebook), mTable, NULL);
        mPageNum = gtk_notebook_get_current_page(GTK_NOTEBOOK(gCtx->notebook));
        
        gtk_table_set_col_spacing(GTK_TABLE(mTable), 1, 0);
        gtk_widget_show(mTable);

        
        readmeContents = GetReadmeContents();
        if (!readmeContents)
        {
            err = ErrorHandler(E_EMPTY_README);
            goto BAIL;
        }

        
#if defined(MOZ_WIDGET_GTK2)
        GtkWidget *text = gtk_scrolled_window_new (NULL, NULL);
        GtkWidget *textview = gtk_text_view_new();
        GtkTextBuffer *textbuffer;

        textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
        gtk_text_buffer_set_text (textbuffer, readmeContents, -1);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (text),
                       GTK_POLICY_AUTOMATIC,
                       GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (text), textview);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(textview), FALSE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
        gtk_table_attach_defaults(GTK_TABLE(mTable), text, 1, 2, 0, 1);
        gtk_widget_show_all(text);
#endif

        mWidgetsInit = TRUE;
    }
    else
    {
        gtk_notebook_set_page(GTK_NOTEBOOK(gCtx->notebook), mPageNum);
        gtk_widget_show(mTable);
    }

    
    gCtx->nextID = gtk_signal_connect(GTK_OBJECT(gCtx->next), "clicked",
                   GTK_SIGNAL_FUNC(nsWelcomeDlg::Next), gCtx->wdlg);

    GTK_WIDGET_SET_FLAGS(gCtx->next, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(gCtx->next);
    gtk_widget_grab_focus(gCtx->next);

    
    gCtx->nextLabel = gtk_label_new(gCtx->Res("NEXT"));
    gtk_container_add(GTK_CONTAINER(gCtx->next), gCtx->nextLabel);
    gtk_widget_show(gCtx->nextLabel);
    gtk_widget_show(gCtx->next);

BAIL:
    XI_IF_FREE(readmeContents);

    return err;
}

int 
nsWelcomeDlg::Hide()
{
    
    gtk_widget_hide(mTable);

    
    gtk_signal_disconnect(GTK_OBJECT(gCtx->next), gCtx->nextID);
    gtk_container_remove(GTK_CONTAINER(gCtx->next), gCtx->nextLabel);
    gtk_widget_hide(gCtx->next);

    return OK;
}

int
nsWelcomeDlg::SetReadmeFile(char *aReadmeFile)
{
    if (!aReadmeFile)
        return E_PARAM;

    mReadmeFile = aReadmeFile;

    return OK;
}

char *
nsWelcomeDlg::GetReadmeFile()
{
    if (mReadmeFile)
        return mReadmeFile;

    return NULL;
}

char *
nsWelcomeDlg::GetReadmeContents()
{
    char *buf = NULL;
    FILE *fd = NULL;
    struct stat attr;
    int buflen;

    DUMP(mReadmeFile);
    if (!mReadmeFile)
        return NULL;
   
    
    fd = fopen(mReadmeFile, "r");
    if (!fd) return NULL;
    DUMP("readme fopen");

    
    if (0 != stat(mReadmeFile, &attr)) goto BAIL;
    if (attr.st_size == 0) goto BAIL;
    DUMP("readme fstat");

    
    buflen = sizeof(char) * (attr.st_size + 1);
    buf = (char *) malloc(buflen);
    if (!buf) goto BAIL;
    memset(buf, 0, buflen);
    DUMP("readme buf malloc");

    
    if (attr.st_size != ((int) fread(buf, sizeof(char), attr.st_size, fd))) 
        XI_IF_FREE(buf);
    DUMP("readme fread");

BAIL:
    
    fclose(fd);
    DUMP("readme close");

    return buf;
}
