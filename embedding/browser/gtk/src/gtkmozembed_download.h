




































#ifndef gtkmozembed_download_h
#define gtkmozembed_download_h
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <gtk/gtk.h>
#ifdef MOZILLA_CLIENT
#include "nscore.h"
#else 
#ifndef nscore_h__


#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define NS_HIDDEN __attribute__((visibility("hidden")))
#else
#define NS_HIDDEN
#endif
#define NS_FROZENCALL
#define NS_EXPORT_(type) type
#define NS_IMPORT_(type) type
#endif 
#endif 
#ifdef XPCOM_GLUE
#define GTKMOZEMBED_API(type, name, params) \
  typedef type (NS_FROZENCALL * name##Type) params; \
  extern name##Type name NS_HIDDEN;
#else 
#ifdef _IMPL_GTKMOZEMBED
#define GTKMOZEMBED_API(type, name, params) NS_EXPORT_(type) name params;
#else
#define GTKMOZEMBED_API(type,name, params) NS_IMPORT_(type) name params;
#endif
#endif 
#define GTK_TYPE_MOZ_EMBED_DOWNLOAD             (gtk_moz_embed_download_get_type())
#define GTK_MOZ_EMBED_DOWNLOAD(obj)             GTK_CHECK_CAST((obj), GTK_TYPE_MOZ_EMBED_DOWNLOAD, GtkMozEmbedDownload)
#define GTK_MOZ_EMBED_DOWNLOAD_CLASS(klass)     GTK_CHECK_CLASS_CAST((klass), GTK_TYPE_MOZ_EMBED_DOWNLOAD, GtkMozEmbedDownloadClass)
#define GTK_IS_MOZ_EMBED_DOWNLOAD(obj)          GTK_CHECK_TYPE((obj), GTK_TYPE_MOZ_EMBED_DOWNLOAD)
#define GTK_IS_MOZ_EMBED_DOWNLOAD_CLASS(klass)  GTK_CHECK_CLASS_TYPE((klass), GTK_TYPE_MOZ_EMBED_DOWNLOAD)
typedef struct _GtkMozEmbedDownload      GtkMozEmbedDownload;
typedef struct _GtkMozEmbedDownloadClass GtkMozEmbedDownloadClass;
struct _GtkMozEmbedDownload
{
  GtkObject  object;
  void *data;
};
struct _GtkMozEmbedDownloadClass
{
  GtkObjectClass parent_class;
  void (*started) (GtkMozEmbedDownload* item, gchar **file_name_with_path);
  void (*completed) (GtkMozEmbedDownload* item);
  void (*error) (GtkMozEmbedDownload* item);
  void (*aborted) (GtkMozEmbedDownload* item);
  void (*progress) (GtkMozEmbedDownload* item, gulong downloaded_bytes, gulong total_bytes, gdouble kbps);
};
typedef enum
{
  GTK_MOZ_EMBED_DOWNLOAD_RESUME,
  GTK_MOZ_EMBED_DOWNLOAD_CANCEL,
  GTK_MOZ_EMBED_DOWNLOAD_PAUSE,
  GTK_MOZ_EMBED_DOWNLOAD_RELOAD,
  GTK_MOZ_EMBED_DOWNLOAD_STORE,
  GTK_MOZ_EMBED_DOWNLOAD_RESTORE
} GtkMozEmbedDownloadActions;
GTKMOZEMBED_API(GtkType,             gtk_moz_embed_download_get_type, (void))
GTKMOZEMBED_API(GtkObject *,         gtk_moz_embed_download_new,      (void))
GTKMOZEMBED_API(GtkObject *,         gtk_moz_embed_download_get_lastest_object, (void))
GTKMOZEMBED_API(void,                gtk_moz_embed_download_do_command , (GtkMozEmbedDownload *item, guint command))
GTKMOZEMBED_API(void,                gtk_moz_embed_download_do_command , (GtkMozEmbedDownload *item, guint command))
GTKMOZEMBED_API(void,                gtk_moz_embed_download_do_command , (GtkMozEmbedDownload *item, guint command))
GTKMOZEMBED_API(void,                gtk_moz_embed_download_do_command , (GtkMozEmbedDownload *item, guint command))
GTKMOZEMBED_API(gchar *,             gtk_moz_embed_download_get_file_name , (GtkMozEmbedDownload *item))
GTKMOZEMBED_API(gchar *,             gtk_moz_embed_download_get_url , (GtkMozEmbedDownload *item))
GTKMOZEMBED_API(glong ,              gtk_moz_embed_download_get_progress , (GtkMozEmbedDownload *item))
GTKMOZEMBED_API(glong ,              gtk_moz_embed_download_get_file_size , (GtkMozEmbedDownload *item))
#ifdef __cplusplus
}
#endif 
#endif 
