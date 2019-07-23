








































#include <stdio.h>
#include "gtkmozembed.h"
#include "gtkmozembed_common.h"
#include "gtkmozembedprivate.h"
#include "gtkmozembed_internal.h"
#include "EmbedPrivate.h"
#include "EmbedWindow.h"

#ifdef MOZ_GTKPASSWORD_INTERFACE
#include "EmbedPasswordMgr.h"
#include "nsIPassword.h"
#endif

#include "EmbedGlobalHistory.h"


#include "nsIWebBrowser.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "nsICookieManager.h"
#include "nsIPermissionManager.h"
#include "nsNetCID.h"
#include "nsICookie.h"
#include "nsIX509Cert.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#else
#include "nsStringAPI.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#endif

#include "nsIDOMNavigator.h"
#include "nsIDOMPluginArray.h"
#include "nsIDOMPlugin.h"
#include <plugin/nsIPluginHost.h>
#include "nsIDOMMimeType.h"
#include "nsIObserverService.h"


#include "nsIWebProgressListener.h"


#include "nsICacheService.h"
#include "nsICache.h"

#include "gtkmozembedmarshal.h"
#define NEW_TOOLKIT_STRING(x) g_strdup(NS_ConvertUTF16toUTF8(x).get())
#define GET_TOOLKIT_STRING(x) NS_ConvertUTF16toUTF8(x).get()
#define GET_OBJECT_CLASS_TYPE(x) G_OBJECT_CLASS_TYPE(x)

#define UNACCEPTABLE_CRASHY_GLIB_ALLOCATION(newed) PR_BEGIN_MACRO \
  /* OOPS this code is using a glib allocation function which     \
   * will cause the application to crash when it runs out of      \
   * memory. This is not cool. either g_try methods should be     \
   * used or malloc, or new (note that gecko /will/ be replacing  \
   * its operator new such that new will not throw exceptions).   \
   * XXX please fix me.                                           \
   */                                                             \
  if (!newed) {                                                   \
  }                                                               \
  PR_END_MACRO

#define ALLOC_NOT_CHECKED(newed) PR_BEGIN_MACRO \
  /* This might not crash, but the code probably isn't really \
   * designed to handle it, perhaps the code should be fixed? \
   */                                                         \
  if (!newed) {                                               \
  }                                                           \
  PR_END_MACRO


static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);



static void
gtk_moz_embed_common_class_init(GtkMozEmbedCommonClass *klass);

static void
gtk_moz_embed_common_init(GtkMozEmbedCommon *embed);

static GtkObjectClass *common_parent_class = NULL;



static void
gtk_moz_embed_common_destroy(GtkObject *object);

guint moz_embed_common_signals[COMMON_LAST_SIGNAL] = { 0 };


GtkType
gtk_moz_embed_common_get_type(void)
{
  static GtkType moz_embed_common_type = 0;
  if (!moz_embed_common_type)
  {
    static const GtkTypeInfo moz_embed_common_info =
    {
      "GtkMozEmbedCommon",
      sizeof(GtkMozEmbedCommon),
      sizeof(GtkMozEmbedCommonClass),
      (GtkClassInitFunc)gtk_moz_embed_common_class_init,
      (GtkObjectInitFunc)gtk_moz_embed_common_init,
      0,
      0,
      0
    };
    moz_embed_common_type = gtk_type_unique(GTK_TYPE_BIN, &moz_embed_common_info);
  }
  return moz_embed_common_type;
}

static void
gtk_moz_embed_common_class_init(GtkMozEmbedCommonClass *klass)
{
  GtkObjectClass     *object_class;
  object_class    = GTK_OBJECT_CLASS(klass);
  common_parent_class = (GtkObjectClass *)gtk_type_class(gtk_object_get_type());
  object_class->destroy = gtk_moz_embed_common_destroy;
  moz_embed_common_signals[COMMON_CERT_ERROR] =
    gtk_signal_new("certificate-error",
                   GTK_RUN_LAST,
                   GET_OBJECT_CLASS_TYPE(klass),
                   GTK_SIGNAL_OFFSET(GtkMozEmbedCommonClass,
                                     certificate_error),
                   gtkmozembed_BOOL__POINTER_UINT,
                   G_TYPE_BOOLEAN,
                   2,
                   GTK_TYPE_POINTER,
                   GTK_TYPE_UINT);
  moz_embed_common_signals[COMMON_SELECT_LOGIN] =
    gtk_signal_new("select-login",
                   GTK_RUN_LAST,
                   GET_OBJECT_CLASS_TYPE(klass),
                   GTK_SIGNAL_OFFSET(GtkMozEmbedCommonClass,
                                     select_login),
                   gtk_marshal_INT__POINTER,
                   G_TYPE_INT,
                   1,
                   G_TYPE_POINTER);
  moz_embed_common_signals[COMMON_REMEMBER_LOGIN] =
    gtk_signal_new("remember-login",
                   GTK_RUN_LAST,
                   GET_OBJECT_CLASS_TYPE(klass),
                   GTK_SIGNAL_OFFSET(GtkMozEmbedCommonClass,
                                     remember_login),
                   gtkmozembed_INT__VOID,
                   G_TYPE_INT, 0);
  
  moz_embed_common_signals[COMMON_ASK_COOKIE] =
    gtk_signal_new("ask-cookie",
                   GTK_RUN_FIRST,
                   GET_OBJECT_CLASS_TYPE(klass),
                   GTK_SIGNAL_OFFSET(GtkMozEmbedCommonClass, ask_cookie),
                   gtkmozembed_VOID__POINTER_INT_STRING_STRING_STRING_STRING_STRING_BOOLEAN_INT,
                   G_TYPE_NONE, 9,
                   GTK_TYPE_POINTER,
                   GTK_TYPE_INT,
                   GTK_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                   GTK_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                   GTK_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                   GTK_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                   GTK_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                   GTK_TYPE_BOOL,
                   GTK_TYPE_INT);






























































}

static void
gtk_moz_embed_common_init(GtkMozEmbedCommon *common)
{
  
  
  common->data = nsnull;
  EmbedCommon *priv = EmbedCommon::GetInstance();
  priv->mCommon = GTK_OBJECT(common);
  common->data = priv;
  EmbedGlobalHistory::GetInstance();
}

static void
gtk_moz_embed_common_destroy(GtkObject *object)
{
  g_return_if_fail(object != NULL);
  g_return_if_fail(GTK_IS_MOZ_EMBED_COMMON(object));
  GtkMozEmbedCommon  *embed = nsnull;
  EmbedCommon *commonPrivate = nsnull;
  embed = GTK_MOZ_EMBED_COMMON(object);
  commonPrivate = (EmbedCommon *)embed->data;
  if (commonPrivate) {
    delete commonPrivate;
    embed->data = NULL;
  }
}

GtkWidget *
gtk_moz_embed_common_new(void)
{
  GtkWidget *widget = (GtkWidget*) gtk_type_new(gtk_moz_embed_common_get_type());
  gtk_widget_set_name(widget, "gtkmozembedcommon");
  return (GtkWidget *) widget;
}

gboolean
gtk_moz_embed_common_set_pref(GtkType type, gchar *name, gpointer value)
{
  g_return_val_if_fail (name != NULL, FALSE);

  nsCOMPtr<nsIPref> pref = do_GetService(NS_PREF_CONTRACTID);

  if (pref) {
    nsresult rv = NS_ERROR_FAILURE;
    switch (type) {
    case GTK_TYPE_BOOL:
      {
        
        rv = pref->SetBoolPref(name, !!*(int*)value);
        break;
      }
    case GTK_TYPE_INT:
      {
        
        rv = pref->SetIntPref(name, *(int*)value);
        break;
      }
    case GTK_TYPE_STRING:
      {
        g_return_val_if_fail (value, FALSE);
        rv = pref->SetCharPref(name, (gchar*)value);
        break;
      }
    default:
      break;
    }
    return NS_SUCCEEDED(rv);
  }
  return FALSE;
}

gboolean
gtk_moz_embed_common_get_pref(GtkType type, gchar *name, gpointer value)
{
  g_return_val_if_fail (name != NULL, FALSE);

  nsCOMPtr<nsIPref> pref = do_GetService(NS_PREF_CONTRACTID);

  nsresult rv = NS_ERROR_FAILURE;
  if (pref){
    switch (type) {
    case GTK_TYPE_BOOL:
      {
        rv = pref->GetBoolPref(name, (gboolean*)value);
        break;
      }
    case GTK_TYPE_INT:
      {
        rv = pref->GetIntPref(name, (gint*)value);
        break;
      }
    case GTK_TYPE_STRING:
      {
        rv = pref->GetCharPref(name, (gchar**)value);
        break;
      }
    default:
      break;
    }
    return NS_SUCCEEDED(rv);
  }
  return FALSE;
}

gboolean
gtk_moz_embed_common_save_prefs()
{
  nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREF_CONTRACTID);
  g_return_val_if_fail (prefService != nsnull, FALSE);
  if (prefService == nsnull)
    return FALSE;
  nsresult rv = prefService->SavePrefFile (nsnull);
  return NS_SUCCEEDED(rv);
}

gint
gtk_moz_embed_common_get_logins(const char* uri, GList **list)
{
  gint ret = 0;
#ifdef MOZ_GTKPASSWORD_INTERFACE
  EmbedPasswordMgr *passwordManager = EmbedPasswordMgr::GetInstance();
  nsCOMPtr<nsISimpleEnumerator> passwordEnumerator;
  nsresult result = passwordManager->GetEnumerator(getter_AddRefs(passwordEnumerator));
  PRBool enumResult;
  for (passwordEnumerator->HasMoreElements(&enumResult) ;
       enumResult == PR_TRUE ;
       passwordEnumerator->HasMoreElements(&enumResult))
  {
    nsCOMPtr<nsIPassword> nsPassword;
    result = passwordEnumerator->GetNext(getter_AddRefs(nsPassword));
    if (NS_FAILED(result)) {
      
      return ret;
    }
    nsCString host;
    nsPassword->GetHost(host);
    nsCString nsCURI(uri);
    if (uri) {
      if (!StringBeginsWith(nsCURI, host)
          
          )
        continue;
    } else if (!passwordManager->IsEqualToLastHostQuery(host))
      continue;

    if (list) {
      nsString unicodeName;
      nsString unicodePassword;
      nsPassword->GetUser(unicodeName);
      nsPassword->GetPassword(unicodePassword);
      GtkMozLogin * login = g_new0(GtkMozLogin, 1);
      UNACCEPTABLE_CRASHY_GLIB_ALLOCATION(login);
      login->user = ToNewUTF8String(unicodeName);
      ALLOC_NOT_CHECKED(login->user);
      login->pass = ToNewUTF8String(unicodePassword);
      ALLOC_NOT_CHECKED(login->pass);
      login->host = NS_strdup(host.get());
      ALLOC_NOT_CHECKED(login->host);
      login->index = ret;
      *list = g_list_append(*list, login);
    }
    ret++;
  }
#endif
  return ret;
}

gboolean
gtk_moz_embed_common_remove_passwords(const gchar *host, const gchar *user, gint index)
{
#ifdef MOZ_GTKPASSWORD_INTERFACE
  EmbedPasswordMgr *passwordManager = EmbedPasswordMgr::GetInstance();
  if (index >= 0) {
    passwordManager->RemovePasswordsByIndex(index);
  } else {
    passwordManager->RemovePasswords(host, user);
  }
#endif
  return TRUE;
}

gint
gtk_moz_embed_common_get_history_list(GtkMozHistoryItem **GtkHI)
{
  gint count = 0;
  EmbedGlobalHistory *history = EmbedGlobalHistory::GetInstance();
  history->GetContentList(GtkHI, &count);
  return count;
}

gint
gtk_moz_embed_common_remove_history(gchar *url, gint time) {
  nsresult rv;
  
  nsCOMPtr<nsIGlobalHistory2> globalHistory(do_GetService("@mozilla.org/browser/global-history;2"));
  if (!globalHistory) return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIObserver> myHistory = do_QueryInterface(globalHistory, &rv);
  if (!myHistory) return NS_ERROR_NULL_POINTER ;
  if (!url)
    myHistory->Observe(nsnull, "RemoveEntries", nsnull);
  else {
    EmbedGlobalHistory *history = EmbedGlobalHistory::GetInstance();
    PRUnichar *uniurl = ToNewUnicode(NS_ConvertUTF8toUTF16(url));
    rv = history->RemoveEntries(uniurl, time);
    NS_Free(uniurl);
  }
  return 1;
}

GSList*
gtk_moz_embed_common_get_cookie_list(void)
{
  GSList *cookies = NULL;
  nsresult result;
  nsCOMPtr<nsICookieManager> cookieManager =
    do_GetService(NS_COOKIEMANAGER_CONTRACTID);
  nsCOMPtr<nsISimpleEnumerator> cookieEnumerator;
  result = cookieManager->GetEnumerator(getter_AddRefs(cookieEnumerator));
  g_return_val_if_fail(NS_SUCCEEDED(result), NULL);
  PRBool enumResult;
  for (cookieEnumerator->HasMoreElements(&enumResult);
       enumResult == PR_TRUE;
       cookieEnumerator->HasMoreElements(&enumResult))
  {
    GtkMozCookieList *c;
    nsCOMPtr<nsICookie> nsCookie;
    result = cookieEnumerator->GetNext(getter_AddRefs(nsCookie));
    g_return_val_if_fail(NS_SUCCEEDED(result), NULL);
    c = g_new0(GtkMozCookieList, 1);
    UNACCEPTABLE_CRASHY_GLIB_ALLOCATION(c);
    nsCAutoString transfer;
    nsCookie->GetHost(transfer);
    c->domain = g_strdup(transfer.get());
    nsCookie->GetName(transfer);
    c->name = g_strdup(transfer.get());
    nsCookie->GetValue(transfer);
    c->value = g_strdup(transfer.get());
    nsCookie->GetPath(transfer);
    
    if (strchr(c->domain,'.'))
      c->path = g_strdup(g_strconcat("http://*",c->domain,"/",NULL));
    else
      c->path = g_strdup(g_strconcat("http://",c->domain,"/",NULL));
    cookies = g_slist_prepend(cookies, c);
  }
  cookies = g_slist_reverse(cookies);
  return cookies;
}

gint
gtk_moz_embed_common_delete_all_cookies(GSList *deletedCookies)
{
  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

  if (!permissionManager)
    return 1;

  permissionManager->RemoveAll();

  if (!deletedCookies)
    return 1;

  nsCOMPtr<nsICookieManager> cookieManager =
    do_GetService(NS_COOKIEMANAGER_CONTRACTID);

  if (!cookieManager)
    return 1;
  cookieManager->RemoveAll();

  g_slist_free(deletedCookies);
  return 0;
}

unsigned char *
gtk_moz_embed_common_nsx509_to_raw(void *nsIX509Ptr, guint *len)
{
  if (!nsIX509Ptr)
    return NULL;
  unsigned char *data;
  ((nsIX509Cert*)nsIX509Ptr)->GetRawDER(len, (PRUint8 **)&data);
  if (!data)
    return NULL;
  return data;
}

gint
gtk_moz_embed_common_get_plugins_list(GList **pluginArray)
{
  nsresult rv;
  nsCOMPtr<nsIPluginManager> pluginMan =
    do_GetService(kPluginManagerCID, &rv);
  if (NS_FAILED(rv)) {
    g_print("Could not get the plugin manager\n");
    return -1;
  }
  pluginMan->ReloadPlugins(PR_TRUE);  

  nsCOMPtr<nsIPluginHost> pluginHost =
    do_GetService(kPluginManagerCID, &rv);
  if (NS_FAILED(rv))
    return -1;

  PRUint32 aLength;
  pluginHost->GetPluginCount(&aLength);

  if (!pluginArray)
    return (gint)aLength;

  nsIDOMPlugin **aItems = nsnull;
  aItems = new nsIDOMPlugin*[aLength];
  if (!aItems)
    return -1; 

  rv = pluginHost->GetPlugins(aLength, aItems);
  if (NS_FAILED(rv)) {
    delete [] aItems;
    return -1;
  }

  nsString string;
  for (int plugin_index = 0; plugin_index < (gint) aLength; plugin_index++)
  {
    GtkMozPlugin *list_item = g_new0(GtkMozPlugin, 1);
    UNACCEPTABLE_CRASHY_GLIB_ALLOCATION(list_item);

    rv = aItems[plugin_index]->GetName(string);
    if (!NS_FAILED(rv))
      list_item->title = g_strdup(NS_ConvertUTF16toUTF8(string).get());

    aItems[plugin_index]->GetFilename(string);
    if (!NS_FAILED(rv))
      list_item->path = g_strdup(NS_ConvertUTF16toUTF8(string).get());

    nsCOMPtr<nsIDOMMimeType> mimeType;
    PRUint32 mime_count = 0;
    rv = aItems[plugin_index]->GetLength(&mime_count);
    if (NS_FAILED(rv))
      continue;
    
    nsString single_mime;
    string.SetLength(0);
    for (int mime_index = 0; mime_index < mime_count; ++mime_index) {
      rv = aItems[plugin_index]->Item(mime_index, getter_AddRefs(mimeType));
      if (NS_FAILED(rv))
        continue;
      rv = mimeType->GetDescription(single_mime);
      if (!NS_FAILED(rv)) {
        string.Append(single_mime);
        string.AppendLiteral(";");
      }
    }
    
    list_item->type = g_strdup(NS_ConvertUTF16toUTF8(string).get());
    if (!NS_FAILED(rv))
      *pluginArray = g_list_append(*pluginArray, list_item);
  }
  delete [] aItems;
  return (gint)aLength;
}

void
gtk_moz_embed_common_reload_plugins()
{
  nsresult rv;
  nsCOMPtr<nsIPluginManager> pluginMan =
    do_GetService(kPluginManagerCID, &rv);
  pluginMan->ReloadPlugins(PR_TRUE); 
}

guint
gtk_moz_embed_common_get_security_mode(guint sec_state)
{
  GtkMozEmbedSecurityMode sec_mode;

  const guint wpl_security_bits = nsIWebProgressListener::STATE_IS_SECURE |
                                  nsIWebProgressListener::STATE_IS_BROKEN |
                                  nsIWebProgressListener::STATE_IS_INSECURE |
                                  nsIWebProgressListener::STATE_SECURE_HIGH |
                                  nsIWebProgressListener::STATE_SECURE_MED |
                                  nsIWebProgressListener::STATE_SECURE_LOW;

  


  switch (sec_state & wpl_security_bits) {
    case nsIWebProgressListener::STATE_IS_INSECURE:
      sec_mode = GTK_MOZ_EMBED_NO_SECURITY;
      
      break;
    case nsIWebProgressListener::STATE_IS_BROKEN:
      sec_mode = GTK_MOZ_EMBED_NO_SECURITY;
      
      break;
    case nsIWebProgressListener::STATE_IS_SECURE|
      nsIWebProgressListener::STATE_SECURE_HIGH:
      sec_mode = GTK_MOZ_EMBED_HIGH_SECURITY;
      
      break;
    case nsIWebProgressListener::STATE_IS_SECURE|
      nsIWebProgressListener::STATE_SECURE_MED:
      sec_mode = GTK_MOZ_EMBED_MEDIUM_SECURITY;
      
      break;
    case nsIWebProgressListener::STATE_IS_SECURE|
      nsIWebProgressListener::STATE_SECURE_LOW:
      sec_mode = GTK_MOZ_EMBED_LOW_SECURITY;
      
      break;
    default:
      sec_mode = GTK_MOZ_EMBED_UNKNOWN_SECURITY;
      
      break;
  }
  return sec_mode;
}

gint
gtk_moz_embed_common_clear_cache(void)
{
  nsCacheStoragePolicy storagePolicy;

  nsCOMPtr<nsICacheService> cacheService = do_GetService(NS_CACHESERVICE_CONTRACTID);

  if (cacheService)
  {
    
    storagePolicy = nsICache::STORE_ANYWHERE;
    cacheService->EvictEntries(storagePolicy);
    return 0;
  }
  return 1;
}

gboolean
gtk_moz_embed_common_observe(const gchar* service_id,
                             gpointer object,
                             const gchar* topic,
                             gunichar* data)
{
  nsresult rv;
  if (service_id) {
    nsCOMPtr<nsISupports> service = do_GetService(service_id, &rv);
    NS_ENSURE_SUCCESS(rv, FALSE);
    nsCOMPtr<nsIObserver> Observer = do_QueryInterface(service, &rv);
    NS_ENSURE_SUCCESS(rv, FALSE);
    rv = Observer->Observe((nsISupports*)object, topic, (PRUnichar*)data);
  } else {
    
    nsCOMPtr<nsIObserverService> obsService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    if (obsService)
      rv = obsService->NotifyObservers((nsISupports*)object, topic, (PRUnichar*)data);
  }
  return NS_FAILED(rv) ? FALSE : TRUE;
}
