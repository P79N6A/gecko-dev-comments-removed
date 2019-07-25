






































#include <stdio.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include "progressui.h"
#include "readstrings.h"
#include "errors.h"

#define TIMER_INTERVAL 100

static float    sProgressVal;  
static gboolean sQuit = FALSE;
static gboolean sEnableUI;
static guint    sTimerID;

static GtkWidget *sWin;
static GtkWidget *sLabel;
static GtkWidget *sProgressBar;

static const char *sProgramPath;

static gboolean
UpdateDialog(gpointer data)
{
  if (sQuit)
  {
    gtk_widget_hide(sWin);
    gtk_main_quit();
  }

  float progress = sProgressVal;

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sProgressBar),
                                progress / 100.0);

  return TRUE;
}

static gboolean
OnDeleteEvent(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  return TRUE;
}

int
InitProgressUI(int *pargc, char ***pargv)
{
  sProgramPath = (*pargv)[0];

  sEnableUI = gtk_init_check(pargc, pargv);
  return 0;
}

int
ShowProgressUI()
{
  if (!sEnableUI)
    return -1;

  
  
  
  usleep(500000);

  if (sQuit || sProgressVal > 70.0f)
    return 0;

  char ini_path[PATH_MAX];
  snprintf(ini_path, sizeof(ini_path), "%s.ini", sProgramPath);

  StringTable strings;
  if (ReadStrings(ini_path, &strings) != OK)
    return -1;

  sWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  if (!sWin)
    return -1;

  static GdkPixbuf *pixbuf;
  char icon_path[PATH_MAX];
  snprintf(icon_path, sizeof(icon_path), "%s.png", sProgramPath);

  g_signal_connect(G_OBJECT(sWin), "delete_event",
                   G_CALLBACK(OnDeleteEvent), NULL);

  gtk_window_set_title(GTK_WINDOW(sWin), strings.title);
  gtk_window_set_type_hint(GTK_WINDOW(sWin), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_position(GTK_WINDOW(sWin), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable(GTK_WINDOW(sWin), FALSE);
  gtk_window_set_decorated(GTK_WINDOW(sWin), TRUE);
  gtk_window_set_deletable(GTK_WINDOW(sWin),FALSE);
  pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
  gtk_window_set_icon(GTK_WINDOW(sWin), pixbuf);
  g_object_unref(pixbuf);

  GtkWidget *vbox = gtk_vbox_new(TRUE, 6);
  sLabel = gtk_label_new(strings.info);
  gtk_misc_set_alignment(GTK_MISC(sLabel), 0.0f, 0.0f);
  sProgressBar = gtk_progress_bar_new();

  gtk_box_pack_start(GTK_BOX(vbox), sLabel, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), sProgressBar, TRUE, TRUE, 0);

  sTimerID = g_timeout_add(TIMER_INTERVAL, UpdateDialog, NULL);

  gtk_container_set_border_width(GTK_CONTAINER(sWin), 10);
  gtk_container_add(GTK_CONTAINER(sWin), vbox);
  gtk_widget_show_all(sWin);

  gtk_main();
  return 0;
}


void
QuitProgressUI()
{
  sQuit = TRUE;
}


void
UpdateProgressUI(float progress)
{
  sProgressVal = progress;  
}
