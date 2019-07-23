






































#ifndef gtkmozembed_common_h
#define gtkmozembed_common_h
#include "gtkmozembed.h"
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
#define GTK_TYPE_MOZ_EMBED_COMMON             (gtk_moz_embed_common_get_type())
#define GTK_MOZ_EMBED_COMMON(obj)             GTK_CHECK_CAST((obj), GTK_TYPE_MOZ_EMBED_COMMON, GtkMozEmbedCommon)
#define GTK_MOZ_EMBED_COMMON_CLASS(klass)     GTK_CHECK_CLASS_CAST((klass), GTK_TYPE_MOZ_EMBED_COMMON, GtkMozEmbedCommonClass)
#define GTK_IS_MOZ_EMBED_COMMON(obj)          GTK_CHECK_TYPE((obj), GTK_TYPE_MOZ_EMBED_COMMON)
#define GTK_IS_MOZ_EMBED_COMMON_CLASS(klass)  GTK_CHECK_CLASS_TYPE((klass), GTK_TYPE_MOZ_EMBED_COMMON)
typedef struct _GtkMozEmbedCommon      GtkMozEmbedCommon;
typedef struct _GtkMozEmbedCommonClass GtkMozEmbedCommonClass;
struct _GtkMozEmbedCommon
{
  GtkBin object;
  void   *data;
};
struct _GtkMozEmbedCommonClass
{
  GtkBinClass parent_class;
  gboolean (* certificate_error)        (GObject *, GObject*, guint);
  gint     (* select_login)             (GObject *, GList*);
  gint     (* remember_login)           (GObject *);
  void     (* ask_cookie)               (GObject * , gint , const gchar * , const gchar * , const gchar * ,
                                         const gchar *, const gchar * , gboolean , gint , GObject *);









};
typedef enum
{
  GTK_MOZ_EMBED_CERT_VERIFIED_OK           = 0x0000,
  GTK_MOZ_EMBED_CERT_UNTRUSTED             = 0x0001,
  GTK_MOZ_EMBED_CERT_NOT_VERIFIED_UNKNOWN  = 0x0002,
  GTK_MOZ_EMBED_CERT_EXPIRED               = 0x0004,
  GTK_MOZ_EMBED_CERT_REVOKED               = 0x0008,
  GTK_MOZ_EMBED_UNKNOWN_CERT               = 0x0010,
  GTK_MOZ_EMBED_CERT_ISSUER_UNTRUSTED      = 0x0020,
  GTK_MOZ_EMBED_CERT_ISSUER_UNKNOWN        = 0x0040,
  GTK_MOZ_EMBED_CERT_INVALID_CA            = 0x0080
} GtkMozEmbedCertificateType;
typedef enum
{
  GTK_MOZ_EMBED_LOGIN_REMEMBER_FOR_THIS_SITE,
  GTK_MOZ_EMBED_LOGIN_REMEMBER_FOR_THIS_SERVER,
  GTK_MOZ_EMBED_LOGIN_NOT_NOW,
  GTK_MOZ_EMBED_LOGIN_NEVER_FOR_SITE,
  GTK_MOZ_EMBED_LOGIN_NEVER_FOR_SERVER
} GtkMozEmbedLoginType;










typedef enum
{
  GTK_MOZ_EMBED_NO_SECURITY = 0,
  GTK_MOZ_EMBED_LOW_SECURITY,
  GTK_MOZ_EMBED_MEDIUM_SECURITY,
  GTK_MOZ_EMBED_HIGH_SECURITY,
  GTK_MOZ_EMBED_UNKNOWN_SECURITY
} GtkMozEmbedSecurityMode;

typedef struct _GtkMozCookieList GtkMozCookieList;
struct _GtkMozCookieList
{
    gchar *domain; 
    gchar *name;   
    gchar *value;  
    gchar *path;   
};
typedef struct _GtkMozEmbedCookie GtkMozEmbedCookie;
struct _GtkMozEmbedCookie
{
    gboolean remember_decision;
    gboolean accept;
};



typedef struct _GtkMozPlugin GtkMozPlugin;
struct _GtkMozPlugin
{
    gchar *title;  
    gchar *path;   
    gchar *type;   
    gboolean isDisabled; 
};

typedef struct _GtkMozLogin GtkMozLogin;
struct _GtkMozLogin
{
    const gchar *user; 
    const gchar *pass; 
    const gchar *host; 
    guint index;
};

GTKMOZEMBED_API(GtkType,    gtk_moz_embed_common_get_type,          (void))
GTKMOZEMBED_API(GtkWidget*, gtk_moz_embed_common_new,               (void))
GTKMOZEMBED_API(gboolean,   gtk_moz_embed_common_set_pref,          (GtkType type, gchar*, gpointer))
GTKMOZEMBED_API(gboolean,   gtk_moz_embed_common_get_pref,          (GtkType type, gchar*, gpointer))
GTKMOZEMBED_API(gboolean,   gtk_moz_embed_common_save_prefs,        (void))
GTKMOZEMBED_API(gboolean,   gtk_moz_embed_common_login,             (GtkWidget *embed, const gchar* username))
GTKMOZEMBED_API(gboolean,   gtk_moz_embed_common_remove_passwords,  (const gchar *host, const gchar *user, gint index))
GTKMOZEMBED_API(gint,       gtk_moz_embed_common_get_logins,        (const char* uri, GList **list))
GTKMOZEMBED_API(gint,       gtk_moz_embed_common_get_history_list,  (GtkMozHistoryItem **GtkHI))
GTKMOZEMBED_API(gint,       gtk_moz_embed_common_remove_history,    (gchar *url, gint time))
GTKMOZEMBED_API(GSList*,    gtk_moz_embed_common_get_cookie_list,   (void))
GTKMOZEMBED_API(gint,       gtk_moz_embed_common_delete_all_cookies,(GSList *deletedCookies))
GTKMOZEMBED_API(unsigned char*, gtk_moz_embed_common_nsx509_to_raw, (void *nsIX509Ptr, guint *len))
GTKMOZEMBED_API(gint,       gtk_moz_embed_common_get_plugins_list,  (GList **pluginArray))
GTKMOZEMBED_API(void,       gtk_moz_embed_common_reload_plugins,    (void))
GTKMOZEMBED_API(guint,      gtk_moz_embed_common_get_security_mode, (guint sec_state))
GTKMOZEMBED_API(gint,       gtk_moz_embed_common_clear_cache,       (void))
GTKMOZEMBED_API(gboolean,   gtk_moz_embed_common_observe,           (const gchar*, gpointer, const gchar*, gunichar*))









#ifdef __cplusplus
}
#endif 
#endif 
