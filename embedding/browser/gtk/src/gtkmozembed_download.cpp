






































#include <stdio.h>
#include "gtkmozembed.h"
#include "gtkmozembed_download.h"
#include "gtkmozembedprivate.h"
#include "gtkmozembed_internal.h"
#include "EmbedPrivate.h"
#include "EmbedWindow.h"
#include "EmbedDownloadMgr.h"

#include "nsIWebBrowser.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#else
#include "nsStringAPI.h"
#endif
#ifdef MOZ_WIDGET_GTK2
#include "gtkmozembedmarshal.h"
#define NEW_TOOLKIT_STRING(x) g_strdup(NS_ConvertUTF16toUTF8(x).get())
#define GET_TOOLKIT_STRING(x) NS_ConvertUTF16toUTF8(x).get()
#define GET_OBJECT_CLASS_TYPE(x) G_OBJECT_CLASS_TYPE(x)
#endif 
#ifdef MOZ_WIDGET_GTK

#include <gtkmozarea.h>

#define gtkmozembed_VOID__INT_UINT \
  gtk_marshal_NONE__INT_INT
#define gtkmozembed_VOID__STRING_INT_INT \
  gtk_marshal_NONE__POINTER_INT_INT
#define gtkmozembed_VOID__STRING_INT_UINT \
  gtk_marshal_NONE__POINTER_INT_INT
#define gtkmozembed_VOID__POINTER_INT_POINTER \
  gtk_marshal_NONE__POINTER_INT_POINTER
#define gtkmozembed_BOOL__STRING \
  gtk_marshal_BOOL__POINTER
#define gtkmozembed_VOID__INT_INT_BOOLEAN \
  gtk_marshal_NONE__INT_INT_BOOLEAN
#define G_SIGNAL_TYPE_STATIC_SCOPE 0
#define NEW_TOOLKIT_STRING(x) g_strdup(NS_LossyConvertUTF16toASCII(x).get())
#define GET_TOOLKIT_STRING(x) NS_LossyConvertUTF16toASCII(x).get()
#define GET_OBJECT_CLASS_TYPE(x) (GTK_OBJECT_CLASS(x)->type)
#endif 
static void gtk_moz_embed_download_set_lastest_object (GtkObject *o);
static GtkObject *latest_download_object = nsnull;

guint moz_embed_download_signals[DOWNLOAD_LAST_SIGNAL] = { 0 };
static void
gtk_moz_embed_download_class_init(GtkMozEmbedDownloadClass *klass);
static void
gtk_moz_embed_download_init(GtkMozEmbedDownload *embed);
static void
gtk_moz_embed_download_destroy(GtkObject *object);
GtkObject *
gtk_moz_embed_download_new(void);

GtkType
gtk_moz_embed_download_get_type(void)
{
  static GtkType moz_embed_download_type = 0;
  if (!moz_embed_download_type)
  {
    static const GtkTypeInfo moz_embed_download_info =
    {
      "GtkMozEmbedDownload",
      sizeof(GtkMozEmbedDownload),
      sizeof(GtkMozEmbedDownloadClass),
      (GtkClassInitFunc)gtk_moz_embed_download_class_init,
      (GtkObjectInitFunc)gtk_moz_embed_download_init,
      0,
      0,
      0
    };
    moz_embed_download_type = gtk_type_unique(GTK_TYPE_OBJECT, &moz_embed_download_info);
  }
  return moz_embed_download_type;
}
static void
gtk_moz_embed_download_class_init(GtkMozEmbedDownloadClass *klass)
{
  GtkObjectClass     *object_class;
  object_class    = GTK_OBJECT_CLASS(klass);
  object_class->destroy = gtk_moz_embed_download_destroy;
  
  moz_embed_download_signals[DOWNLOAD_STARTED_SIGNAL] =
    gtk_signal_new("started",
       GTK_RUN_FIRST,
       GET_OBJECT_CLASS_TYPE(klass),
       GTK_SIGNAL_OFFSET(GtkMozEmbedDownloadClass,
             started),
       gtk_marshal_NONE__POINTER,
       GTK_TYPE_NONE, 1,
                   G_TYPE_POINTER);
  moz_embed_download_signals[DOWNLOAD_COMPLETED_SIGNAL] =
    gtk_signal_new("completed",
       GTK_RUN_FIRST,
       GET_OBJECT_CLASS_TYPE(klass),
       GTK_SIGNAL_OFFSET(GtkMozEmbedDownloadClass,
             completed),
       gtk_marshal_NONE__NONE,
       GTK_TYPE_NONE, 0);
  moz_embed_download_signals[DOWNLOAD_FAILED_SIGNAL] =
    gtk_signal_new("error",
       GTK_RUN_FIRST,
       GET_OBJECT_CLASS_TYPE(klass),
       GTK_SIGNAL_OFFSET(GtkMozEmbedDownloadClass,
             error),
       gtk_marshal_NONE__NONE,
       GTK_TYPE_NONE, 0);
  moz_embed_download_signals[DOWNLOAD_DESTROYED_SIGNAL] =
    gtk_signal_new("aborted",
       GTK_RUN_FIRST,
       GET_OBJECT_CLASS_TYPE(klass),
       GTK_SIGNAL_OFFSET(GtkMozEmbedDownloadClass,
             aborted),
       gtk_marshal_NONE__NONE,
       GTK_TYPE_NONE, 0);
  moz_embed_download_signals[DOWNLOAD_PROGRESS_SIGNAL] =
    gtk_signal_new("progress",
       GTK_RUN_FIRST,
       GET_OBJECT_CLASS_TYPE(klass),
       GTK_SIGNAL_OFFSET(GtkMozEmbedDownloadClass,
             progress),
       gtkmozembed_VOID__ULONG_ULONG_ULONG,
       GTK_TYPE_NONE,
       3,
       G_TYPE_ULONG,
       G_TYPE_ULONG,
       G_TYPE_ULONG);
#ifdef MOZ_WIDGET_GTK
  gtk_object_class_add_signals(object_class, moz_embed_download_signals,
             DOWNLOAD_LAST_SIGNAL);
#endif 
}
static void
gtk_moz_embed_download_init(GtkMozEmbedDownload *download)
{
  
  
  download->data = nsnull;
  EmbedDownload *priv = new EmbedDownload();
  download->data = priv;
}
static void
gtk_moz_embed_download_destroy(GtkObject *object)
{
    GtkMozEmbedDownload  *embed;
    EmbedDownload *downloadPrivate;
    g_return_if_fail(object != NULL);
    g_return_if_fail(GTK_IS_MOZ_EMBED_DOWNLOAD(object));
    embed = GTK_MOZ_EMBED_DOWNLOAD(object);
    downloadPrivate = (EmbedDownload *)embed->data;
    if (downloadPrivate) {
        delete downloadPrivate;
        embed->data = NULL;
    }
}
GtkObject *
gtk_moz_embed_download_new(void)
{
  GtkObject *instance = (GtkObject *) gtk_type_new (gtk_moz_embed_download_get_type());
  gtk_moz_embed_download_set_lastest_object (instance);
  return instance;
}
GtkObject *
gtk_moz_embed_download_get_lastest_object (void)
{
  return latest_download_object;
}
static void
gtk_moz_embed_download_set_lastest_object (GtkObject *obj)
{
  latest_download_object = obj;
  return ;
}
void gtk_moz_embed_download_do_command (GtkMozEmbedDownload *item, guint command)
{
  g_return_if_fail(item);
  EmbedDownload *download_priv = (EmbedDownload *) item->data;
  if (!download_priv) return ;
  if (command == GTK_MOZ_EMBED_DOWNLOAD_CANCEL) {
    
    if (download_priv->downloaded_size != -1)
        download_priv->launcher->Cancel (GTK_MOZ_EMBED_STATUS_FAILED_USERCANCELED);
    download_priv->launcher->SetWebProgressListener (nsnull);
  } else if (command == GTK_MOZ_EMBED_DOWNLOAD_RESUME) {
    download_priv->request->Resume ();
    download_priv->isPaused = FALSE;
  } else if (command == GTK_MOZ_EMBED_DOWNLOAD_PAUSE) {
    download_priv->request->Suspend ();
    download_priv->isPaused = TRUE;
  } else if (command == GTK_MOZ_EMBED_DOWNLOAD_RELOAD) {
    if (download_priv->gtkMozEmbedParentWidget) {}
  }
  
}
gchar * gtk_moz_embed_download_get_file_name (GtkMozEmbedDownload *item)
{
  g_return_val_if_fail(item, nsnull);
  EmbedDownload *download_priv = (EmbedDownload *) item->data;
  if (!download_priv) return nsnull;
  return (gchar *) download_priv->file_name;
}
gchar * gtk_moz_embed_download_get_url (GtkMozEmbedDownload *item)
{
  g_return_val_if_fail(item, nsnull);
  EmbedDownload *download_priv = (EmbedDownload *) item->data;
  if (!download_priv) return nsnull;
  
  return (gchar *) download_priv->server;
}
glong gtk_moz_embed_download_get_progress (GtkMozEmbedDownload *item)
{
  g_return_val_if_fail(item, -1);
  EmbedDownload *download_priv = (EmbedDownload *) item->data;
  if (!download_priv) return -1;
  return (glong) download_priv->downloaded_size;
}
glong gtk_moz_embed_download_get_file_size (GtkMozEmbedDownload *item)
{
  g_return_val_if_fail(item, -1);
  EmbedDownload *download_priv = (EmbedDownload *) item->data;
  if (!download_priv) return -1;
  return (glong) download_priv->file_size;
}
