





































#include  <stdio.h>
#include  <string.h>
#include "nscore.h"


#if defined (XP_WIN)
#include <windows.h>
#elif defined (XP_MAC)
#include <Dialogs.h>
#include <TextUtils.h>
#elif defined (MOZ_WIDGET_GTK)
#include <gtk/gtk.h>
#elif defined (XP_OS2)
#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#endif

extern "C" void ShowOSAlert(const char* aMessage);

#if defined (MOZ_WIDGET_GTK)

static int sbAlertDone = 0;









void
ns_gtk_alert_OK_callback(GtkWidget *aWidget, gpointer aData)
{
    GtkWidget *alertDlg = (GtkWidget *) aData;

    if (!alertDlg)
        return;

    gtk_widget_destroy(alertDlg);
    sbAlertDone = 1;
}


















int
NS_gtk_alert(const char *aMessage, const char *aTitle, const char *aOKBtnText)
{
#ifdef DEBUG_dbragg
    printf ("\n*** Now inside NS_gtk_alert *** \n");
#endif 

    GtkWidget *alertDlg = NULL;
    GtkWidget *okBtn = NULL;
    GtkWidget *msgLabel = NULL;
    GtkWidget *packerLbl = NULL;
    GtkWidget *packerBtn = NULL;
    const char *okBtnText = aOKBtnText;
    const char *title = aTitle;

    if (!aMessage)
        return -1;
    if (!aOKBtnText)
        okBtnText = "OK";
    if (!aTitle)
        title = " ";

#ifdef DEBUG_dbragg
    printf("title is: %s\n", title);
#endif

    alertDlg = gtk_dialog_new();
    msgLabel = gtk_label_new(aMessage);
    if (msgLabel)
      gtk_label_set_line_wrap(GTK_LABEL(msgLabel), TRUE);
    okBtn = gtk_button_new_with_label(okBtnText);
    packerLbl = gtk_packer_new();
    packerBtn = gtk_packer_new();
    sbAlertDone = 0;

    if (alertDlg && msgLabel && okBtn && packerBtn && packerLbl)
    {
        
        gtk_packer_set_default_border_width(GTK_PACKER(packerLbl), 20);
        gtk_packer_add_defaults(GTK_PACKER(packerLbl), msgLabel, 
            GTK_SIDE_BOTTOM, GTK_ANCHOR_CENTER, GTK_FILL_X);
        gtk_packer_set_default_border_width(GTK_PACKER(packerBtn), 0);
        gtk_packer_add_defaults(GTK_PACKER(packerBtn), okBtn, 
            GTK_SIDE_BOTTOM, GTK_ANCHOR_CENTER, GTK_FILL_Y);
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(alertDlg)->vbox), 
            packerLbl);
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(alertDlg)->action_area),
            packerBtn);

        gtk_window_set_modal(GTK_WINDOW(alertDlg), TRUE);
        gtk_window_set_title(GTK_WINDOW(alertDlg), title);
        gtk_window_set_position(GTK_WINDOW(alertDlg), GTK_WIN_POS_CENTER);

        
        gtk_signal_connect(GTK_OBJECT(okBtn), "clicked", 
            GTK_SIGNAL_FUNC(ns_gtk_alert_OK_callback), alertDlg);
        GTK_WIDGET_SET_FLAGS(okBtn, GTK_CAN_DEFAULT);
        gtk_widget_grab_default(okBtn);

        
        gtk_widget_show(msgLabel);
        gtk_widget_show(packerLbl);
        gtk_widget_show(okBtn);
        gtk_widget_show(packerBtn);
        gtk_widget_show(alertDlg);
    }
    else
    {
        return -2;
    }

    while (!sbAlertDone)
    {
        while (gtk_events_pending())
            gtk_main_iteration();
    }

    return 0;
}
#endif 



void ShowOSAlert(const char* aMessage)
{
#ifdef DEBUG_dbragg
printf("\n****Inside ShowOSAlert ***\n");	
#endif 

    const PRInt32 max_len = 255;
    char message_copy[max_len+1] = { 0 };
    PRInt32 input_len = strlen(aMessage);
    PRInt32 copy_len = (input_len > max_len) ? max_len : input_len;
    strncpy(message_copy, aMessage, copy_len);
    message_copy[copy_len] = 0;

#if defined (XP_WIN)
    MessageBox(NULL, message_copy, NULL, MB_OK | MB_ICONERROR | MB_SETFOREGROUND );
#elif (XP_MAC)
    short buttonClicked;
    StandardAlert(kAlertStopAlert, c2pstr(message_copy), nil, nil, &buttonClicked);
#elif defined (MOZ_WIDGET_GTK)
    NS_gtk_alert(message_copy, NULL, "OK");
#elif defined (XP_OS2)
    
    PPIB ppib;
    PTIB ptib;
    DosGetInfoBlocks(&ptib, &ppib);
    ppib->pib_ultype = 3;
    HAB hab = WinInitialize(0);
    HMQ hmq = WinCreateMsgQueue(hmq,0);
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, message_copy, "", 0, MB_OK);
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
#endif
    
    
    fprintf(stdout, "%s\n", aMessage);
}
