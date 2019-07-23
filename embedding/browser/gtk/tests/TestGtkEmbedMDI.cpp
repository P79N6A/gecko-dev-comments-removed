
















#include <gnome.h>
#include "gtkmozembed.h"







#define USE_MOZILLA_TEST














#define SAMPLE_PIXMAP


gint eventDelete(GtkWidget *widget,
        GdkEvent *event,gpointer data);
gint eventDestroy(GtkWidget *widget,
        GdkEvent *event,gpointer data);

static void addChild(GtkObject *mdi,gchar *name);
static GtkWidget *setLabel(GnomeMDIChild *child,
        GtkWidget *currentLabel,gpointer data);
static GtkWidget *createView(GnomeMDIChild *child,
        gpointer data);

int main(int argc,char *argv[])
{
    GtkObject *mdi;

    gnome_init("simplemdi","1.0",argc,argv);
    mdi = gnome_mdi_new("simplemdi","Simple MDI");
    gtk_signal_connect(mdi,"destroy",
            GTK_SIGNAL_FUNC(eventDestroy),NULL);

    addChild(mdi,"First");
    addChild(mdi,"Second");
    addChild(mdi,"Third");
    addChild(mdi,"Last");

    gnome_mdi_set_mode(GNOME_MDI(mdi),GNOME_MDI_NOTEBOOK);
    

    gtk_main();
    exit(0);
}
static void addChild(GtkObject *mdi,gchar *name)
{
    GnomeMDIGenericChild *child;

    child = gnome_mdi_generic_child_new(name);
    gnome_mdi_add_child(GNOME_MDI(mdi),
            GNOME_MDI_CHILD(child));

    gnome_mdi_generic_child_set_view_creator(child,
            createView,name);
    gnome_mdi_generic_child_set_label_func(child,setLabel,
            NULL);
    gnome_mdi_add_view(GNOME_MDI(mdi),
            GNOME_MDI_CHILD(child));
}
static GtkWidget *createView(GnomeMDIChild *child,
        gpointer data)
{
#ifdef USE_MOZILLA_TEST
    GtkWidget *browser = gtk_moz_embed_new();
#else
#ifndef SAMPLE_PIXMAP
    GtkWidget *browser = gtk_label_new("lynx 0.01a");
#else
    
    GtkWidget *browser =
      gnome_pixmap_new_from_file("/usr/share/pixmaps/emacs.png");
#endif 
#endif 

    GtkWidget *notebook = gtk_notebook_new();
    char str[80];

    sprintf(str,"View of the\n%s widget",(gchar *)data);

#ifdef USE_MOZILLA_TEST
    gtk_moz_embed_load_url(GTK_MOZ_EMBED(browser), "http://www.mozilla.org");
#endif 

#ifndef SIMPLER_TEST
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), gtk_label_new(str),
			     gtk_label_new("Label"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), browser,
			     gtk_label_new("Mozilla"));
    gtk_widget_show_all(notebook);
    return (notebook);
#else
    gtk_widget_show(browser);
    return (browser);
#endif 
}

static GtkWidget *setLabel(GnomeMDIChild *child,
        GtkWidget *currentLabel,gpointer data)
{
    if(currentLabel == NULL)
        return(gtk_label_new(child->name));

    gtk_label_set_text(GTK_LABEL(currentLabel),
            child->name);
    return(currentLabel);
}
gint eventDestroy(GtkWidget *widget,
        GdkEvent *event,gpointer data) {
    gtk_main_quit();
    return(0);
}
