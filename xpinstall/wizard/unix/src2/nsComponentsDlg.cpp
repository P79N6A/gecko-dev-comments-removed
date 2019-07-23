






































#include "nsComponentsDlg.h"
#include "nsXInstaller.h"
#include "check_on.xpm"
#include "check_off.xpm"
#include <gdk/gdkkeysyms.h>

static nsSetupType     *sCustomST; 
static GtkWidget       *sDescLong;
static gint             sCurrRowSelected;

nsComponentsDlg::nsComponentsDlg() :
    mMsg0(NULL),
    mCompList(NULL)
{
}

nsComponentsDlg::~nsComponentsDlg()
{
    XI_IF_FREE(mMsg0);
    XI_IF_DELETE(mCompList);
}

void
nsComponentsDlg::Back(GtkWidget *aWidget, gpointer aData)
{
    DUMP("Back");
    if (aData != gCtx->cdlg) return;
#ifdef MOZ_WIDGET_GTK
    if (gCtx->bMoving)
    {
        gCtx->bMoving = FALSE;
        return;
    }
#endif

    
    gCtx->cdlg->Hide();

    gCtx->sdlg->Show();

    
}

void
nsComponentsDlg::Next(GtkWidget *aWidget, gpointer aData)
{
    DUMP("Next");
    if (aData != gCtx->cdlg) return;
#ifdef MOZ_WIDGET_GTK
    if (gCtx->bMoving)
    {
        gCtx->bMoving = FALSE;
        return;
    }
#endif

    if (OK != nsSetupTypeDlg::VerifyDiskSpace())
        return;

    
    gCtx->cdlg->Hide();

    
    gCtx->idlg->Show();
#ifdef MOZ_WIDGET_GTK
    gCtx->bMoving = TRUE;
#endif
}

int
nsComponentsDlg::Parse(nsINIParser *aParser)
{
    int err = OK;
    char *showDlg = NULL;
    int bufsize = 0;
    int i, j, compListLen = 0;

    char *currSec = (char *) malloc(strlen(COMPONENT) + 3);
    if (!currSec) return E_MEM;
    char *currDescShort = NULL;
    char *currDescLong = NULL;
    char *currArchive = NULL;
    char *currInstallSizeStr = NULL;
    char *currArchiveSizeStr = NULL;
    char *currAttrStr = NULL;
    char *currURL = NULL;
    char *currDepName = NULL;
    char urlKey[MAX_URL_LEN];
    char dependeeKey[MAX_DEPENDEE_KEY_LEN];
    nsComponent *currComp = NULL;
    nsComponent *currDepComp = NULL;
    nsComponent *currIdxComp = NULL; 
    XI_VERIFY(gCtx);

    
    err = aParser->GetStringAlloc(DLG_COMPONENTS, MSG0, &mMsg0, &bufsize);
    if (err != OK && err != nsINIParser::E_NO_KEY) goto BAIL; else err = OK;

    bufsize = 0;
    err = aParser->GetStringAlloc(DLG_COMPONENTS, TITLE, &mTitle, &bufsize);
    if (err != OK  && err != nsINIParser::E_NO_KEY) goto BAIL; else err = OK;
    if (bufsize == 0)
            XI_IF_FREE(mTitle); 

    mCompList = new nsComponentList();
     
    for (i=0; i<MAX_COMPONENTS; i++)
    {
        currDescShort = NULL;
        currDescLong = NULL;
        currArchive = NULL;
        currInstallSizeStr = NULL;
        currArchiveSizeStr = NULL;
        currAttrStr = NULL;

        sprintf(currSec, COMPONENTd, i);

        err = aParser->GetStringAlloc(currSec, DESC_SHORT, 
                                            &currDescShort, &bufsize);
        if (err != OK && err != nsINIParser::E_NO_SEC) goto BAIL; 
        if (bufsize == 0 || err == nsINIParser::E_NO_SEC)
        {
            err = OK;
            break;
        }

        XI_ERR_BAIL(aParser->GetStringAlloc(currSec, DESC_LONG,
                                            &currDescLong, &bufsize));
        XI_ERR_BAIL(aParser->GetStringAlloc(currSec, ARCHIVE,
                                            &currArchive, &bufsize));
        XI_ERR_BAIL(aParser->GetStringAlloc(currSec, INSTALL_SIZE,  
                                            &currInstallSizeStr, &bufsize));
        XI_ERR_BAIL(aParser->GetStringAlloc(currSec, ARCHIVE_SIZE,  
                                            &currArchiveSizeStr, &bufsize));
        err = aParser->GetStringAlloc(currSec, ATTRIBUTES,
                                      &currAttrStr, &bufsize);
        if (err != OK && err != nsINIParser::E_NO_KEY) goto BAIL; else err = OK;

        currComp = new nsComponent();
        if (!currComp) return E_MEM;

        currComp->SetIndex(i);
        currComp->SetDescShort(currDescShort);
        currComp->SetDescLong(currDescLong);
        currComp->SetArchive(currArchive);
        currComp->SetInstallSize(atoi(currInstallSizeStr));
        currComp->SetArchiveSize(atoi(currArchiveSizeStr));
        if (NULL != strstr(currAttrStr, SELECTED_ATTR))
        { 
            currComp->SetSelected();
            currComp->DepAddRef();
        }
        else
            currComp->SetUnselected();
        if (NULL != strstr(currAttrStr, INVISIBLE_ATTR))
            currComp->SetInvisible();
        else
            currComp->SetVisible();
        if (NULL != strstr(currAttrStr, LAUNCHAPP_ATTR))
            currComp->SetLaunchApp();
        else
            currComp->SetDontLaunchApp();
        if (NULL != strstr(currAttrStr, DOWNLOAD_ONLY_ATTR))
            currComp->SetDownloadOnly();

        
        for (j = 0; j < MAX_URLS; j++)
        {
            sprintf(urlKey, URLd, j);
            bufsize = 0;
            err = aParser->GetStringAlloc(currSec, urlKey, &currURL, &bufsize);
            if (err == nsINIParser::E_NO_KEY || bufsize == 0)  
                break;
            if (err != OK) goto BAIL; else err = OK;
            currComp->SetURL(currURL, j);
        }

        XI_ERR_BAIL(mCompList->AddComponent(currComp));
    }

    compListLen = mCompList->GetLength();
    if (0 == compListLen)
    {
        XI_IF_DELETE(mCompList);
        err = E_NO_COMPONENTS;
        goto BAIL;
    }

    
    for (i = 0; i < compListLen; i++)
    {
        memset(currSec, 0, strlen(COMPONENT) + 3);
        sprintf(currSec, COMPONENTd, i);

        currIdxComp = mCompList->GetCompByIndex(i);
        if (!currIdxComp)
            continue;

        for (j = 0; j < MAX_COMPONENTS; j++)
        {
            currDepComp = NULL;
            memset(dependeeKey, 0, MAX_DEPENDEE_KEY_LEN);
            sprintf(dependeeKey, DEPENDEEd, j);

            err = aParser->GetStringAlloc(currSec, dependeeKey, 
                &currDepName, &bufsize);
            if (bufsize == 0 || err != nsINIParser::OK || !currDepName) 
            {
                err = OK;
                break; 
            }
            
            currDepComp = mCompList->GetCompByShortDesc(currDepName);
            if (!currDepComp) 
                continue;

            currIdxComp->AddDependee(currDepName);
        }
    }

BAIL:
    XI_IF_FREE(currSec);

    return err;
}

int
nsComponentsDlg::Show()
{
    int err = OK;
    int customSTIndex = 0, i;
    int numRows = 0;
    int currRow = 0;
    GtkWidget *hbox = NULL;

    XI_VERIFY(gCtx);
    XI_VERIFY(gCtx->notebook);

    if (mWidgetsInit == FALSE)
    {
        customSTIndex = gCtx->sdlg->GetNumSetupTypes();
        sCustomST = gCtx->sdlg->GetSetupTypeList();
        for (i=1; i<customSTIndex; i++)
            sCustomST = sCustomST->GetNext();
        DUMP(sCustomST->GetDescShort());

        
        mTable = gtk_table_new(5, 1, FALSE);
        gtk_notebook_append_page(GTK_NOTEBOOK(gCtx->notebook), mTable, NULL);
        mPageNum = gtk_notebook_get_current_page(GTK_NOTEBOOK(gCtx->notebook));
        gtk_widget_show(mTable);

        
        
        GtkWidget *msg0 = gtk_label_new(mMsg0);
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), msg0, FALSE, FALSE, 0);
        gtk_widget_show(hbox);
        gtk_table_attach(GTK_TABLE(mTable), hbox, 0, 1, 1, 2,
                         static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
			             GTK_FILL, 20, 20);
        gtk_widget_show(msg0);

        
        GtkWidget *list = NULL;
        GtkWidget *scrollwin = NULL;
        GtkStyle *style = NULL;
        GdkBitmap *ch_mask = NULL;
        GdkPixmap *checked = NULL;
        GdkBitmap *un_mask = NULL;
        GdkPixmap *unchecked = NULL;
        gchar *dummy[2] = { " ", " " };
        nsComponent *currComp = sCustomST->GetComponents()->GetHead();
        GtkWidget *descLongTable = NULL;
        GtkWidget *frame = NULL;

        scrollwin = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

        list = gtk_clist_new(2);
        gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_BROWSE);
        gtk_clist_column_titles_hide(GTK_CLIST(list));
        gtk_clist_set_column_width(GTK_CLIST(list), 0, 20);
        gtk_clist_set_column_width(GTK_CLIST(list), 1, 200);
        gtk_clist_set_row_height(GTK_CLIST(list), 17);
        
        
        numRows = sCustomST->GetComponents()->GetLengthVisible();
        for (i = 0; i < numRows; i++)
            gtk_clist_append(GTK_CLIST(list), dummy);
    
        style = gtk_widget_get_style(gCtx->window);
        checked = gdk_pixmap_create_from_xpm_d(gCtx->window->window, &ch_mask, 
                  &style->bg[GTK_STATE_NORMAL], (gchar **)check_on_xpm);
        unchecked = gdk_pixmap_create_from_xpm_d(gCtx->window->window, &un_mask,
                    &style->bg[GTK_STATE_NORMAL], (gchar **)check_off_xpm);

        while ((currRow < numRows) && currComp) 
        {
            if (!currComp->IsInvisible())
            {
                if (currComp->IsSelected()) 
                    gtk_clist_set_pixmap(GTK_CLIST(list), currRow, 0, 
                                         checked, ch_mask);
                else
                    gtk_clist_set_pixmap(GTK_CLIST(list), currRow, 0, 
                                         unchecked, un_mask);

                gtk_clist_set_text(GTK_CLIST(list), currRow, 1,
                                   currComp->GetDescShort());
                currRow++;
            }

            currComp = sCustomST->GetComponents()->GetNext();
        }

        
        sCurrRowSelected = 0; 

        gtk_signal_connect(GTK_OBJECT(list), "select_row",
                           GTK_SIGNAL_FUNC(RowSelected), NULL);
        gtk_signal_connect(GTK_OBJECT(list), "key_press_event",
                           GTK_SIGNAL_FUNC(KeyPressed), NULL);
        gtk_container_add(GTK_CONTAINER(scrollwin), list);
        gtk_widget_show(list);
        gtk_widget_show(scrollwin);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), scrollwin, TRUE, TRUE, 0);
        gtk_widget_show(hbox);
        gtk_table_attach(GTK_TABLE(mTable), hbox, 0, 1, 2, 3,
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
			static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
			 20, 0);

        

        
        descLongTable = gtk_table_new(1, 1, FALSE);
        gtk_widget_show(descLongTable);

        gtk_table_attach(GTK_TABLE(mTable), descLongTable, 0, 1, 4, 5,
            static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
            static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
			20, 20);
        frame = gtk_frame_new(gCtx->Res("DESCRIPTION"));
        gtk_table_attach_defaults(GTK_TABLE(descLongTable), frame, 0, 1, 0, 1);
        gtk_widget_show(frame);

        sDescLong = gtk_label_new(
            sCustomST->GetComponents()->GetFirstVisible()->GetDescLong());
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), sDescLong, FALSE, FALSE, 20);
        gtk_widget_show(hbox);

        gtk_table_attach_defaults(GTK_TABLE(descLongTable), hbox, 0, 1, 0, 1);
        gtk_widget_show(sDescLong);

        mWidgetsInit = TRUE;
    }
    else
    {
        gtk_notebook_set_page(GTK_NOTEBOOK(gCtx->notebook), mPageNum);
        gtk_widget_show(mTable);
    }

    
    gCtx->backID = gtk_signal_connect(GTK_OBJECT(gCtx->back), "clicked",
                   GTK_SIGNAL_FUNC(nsComponentsDlg::Back), gCtx->cdlg);
    gCtx->nextID = gtk_signal_connect(GTK_OBJECT(gCtx->next), "clicked",
                   GTK_SIGNAL_FUNC(nsComponentsDlg::Next), gCtx->cdlg);

    
    gCtx->backLabel = gtk_label_new(gCtx->Res("BACK"));
    gCtx->nextLabel = gtk_label_new(gCtx->Res("NEXT"));
    gtk_container_add(GTK_CONTAINER(gCtx->back), gCtx->backLabel);
    gtk_container_add(GTK_CONTAINER(gCtx->next), gCtx->nextLabel);
    gtk_widget_show(gCtx->backLabel);
    gtk_widget_show(gCtx->nextLabel);
    gtk_widget_show(gCtx->back);
    gtk_widget_show(gCtx->next);

    GTK_WIDGET_SET_FLAGS(gCtx->next, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(gCtx->next);
    gtk_widget_grab_focus(gCtx->next);

    return err;
}

int
nsComponentsDlg::Hide()
{
    gtk_widget_hide(mTable);

    
    gtk_signal_disconnect(GTK_OBJECT(gCtx->back), gCtx->backID);
    gtk_signal_disconnect(GTK_OBJECT(gCtx->next), gCtx->nextID);

    gtk_container_remove(GTK_CONTAINER(gCtx->back), gCtx->backLabel); 
    gtk_container_remove(GTK_CONTAINER(gCtx->next), gCtx->nextLabel); 

    gtk_widget_hide(gCtx->back);
    gtk_widget_hide(gCtx->next);

    return OK;
}

int
nsComponentsDlg::SetMsg0(char *aMsg)
{
    if (!aMsg)
        return E_PARAM;

    mMsg0 = aMsg;
    
    return OK;
}

char *
nsComponentsDlg::GetMsg0()
{
    if (mMsg0)
        return mMsg0;

    return NULL;
}

int
nsComponentsDlg::SetCompList(nsComponentList *aCompList)
{
    if (!aCompList)
        return E_PARAM;

    mCompList = aCompList;

    return OK;
}

nsComponentList *
nsComponentsDlg::GetCompList()
{
    if (mCompList)
        return mCompList;

    return NULL;
}

void
nsComponentsDlg::RowSelected(GtkWidget *aWidget, gint aRow, gint aColumn,
                             GdkEventButton *aEvent, gpointer aData)
{
    DUMP("RowSelected");

    sCurrRowSelected = aRow;

    ToggleRowSelection(aWidget, aRow, aColumn);
}

gboolean
nsComponentsDlg::KeyPressed(GtkWidget *aWidget, GdkEventKey *aEvent, 
                            gpointer aData)
{
  DUMP("KeyPressed");

  if (aEvent->keyval == GDK_space)
      ToggleRowSelection(aWidget, sCurrRowSelected, 0);

  return FALSE;
}

void
nsComponentsDlg::ToggleRowSelection(GtkWidget *aWidget, gint aRow, 
                                    gint aColumn)
{
    int numRows = 0, currRow = 0;
    GtkStyle *style = NULL;
    GdkBitmap *ch_mask = NULL;
    GdkPixmap *checked = NULL;
    GdkBitmap *un_mask = NULL;
    GdkPixmap *unchecked = NULL;
    nsComponent *currComp = sCustomST->GetComponents()->GetHead();
    
    style = gtk_widget_get_style(gCtx->window);
    checked = gdk_pixmap_create_from_xpm_d(gCtx->window->window, &ch_mask, 
              &style->bg[GTK_STATE_NORMAL], (gchar **)check_on_xpm);
    unchecked = gdk_pixmap_create_from_xpm_d(gCtx->window->window, &un_mask,
                &style->bg[GTK_STATE_NORMAL], (gchar **)check_off_xpm);

    numRows = sCustomST->GetComponents()->GetLengthVisible();
    while ((currRow < numRows) && currComp) 
    {
        if (!currComp->IsInvisible())
        {
            if (aRow == currRow)
            {
                
                gtk_label_set_text(GTK_LABEL(sDescLong),
                                   currComp->GetDescLong());
                gtk_widget_show(sDescLong);

                
                if (aColumn != 0)
                   break;

                if (currComp->IsSelected())
                {
                    DUMP("Toggling off...");
                    currComp->SetUnselected();
                }
                else
                {
                    DUMP("Toggling on...");
                    currComp->SetSelected();
                }
                currComp->ResolveDependees(currComp->IsSelected(),
                                            sCustomST->GetComponents());
                break;
            }
            currRow++;
        }
        currComp = sCustomST->GetComponents()->GetNext();
    }

    
    currRow = 0;
    currComp = sCustomST->GetComponents()->GetHead();
    while ((currRow < numRows) && currComp) 
    {
        if (!currComp->IsInvisible())
        {
            if (currComp->IsSelected())
            {
                gtk_clist_set_pixmap(GTK_CLIST(aWidget), currRow, 0, 
                                     checked, ch_mask);
            }
            else
            {
                gtk_clist_set_pixmap(GTK_CLIST(aWidget), currRow, 0, 
                                     unchecked, un_mask);
            }
            currRow++;
        }
        currComp = sCustomST->GetComponents()->GetNext();
    }
}
