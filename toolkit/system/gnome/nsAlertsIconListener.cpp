




#include "nsAlertsIconListener.h"
#include "imgIContainer.h"
#include "imgILoader.h"
#include "imgIRequest.h"
#include "nsNetUtil.h"
#include "nsIImageToPixbuf.h"
#include "nsIStringBundle.h"
#include "nsIObserverService.h"
#include "nsCRT.h"

#include <dlfcn.h>
#include <gdk/gdk.h>

static bool gHasActions = false;
static bool gHasCaps = false;

void* nsAlertsIconListener::libNotifyHandle = nullptr;
bool nsAlertsIconListener::libNotifyNotAvail = false;
nsAlertsIconListener::notify_is_initted_t nsAlertsIconListener::notify_is_initted = nullptr;
nsAlertsIconListener::notify_init_t nsAlertsIconListener::notify_init = nullptr;
nsAlertsIconListener::notify_get_server_caps_t nsAlertsIconListener::notify_get_server_caps = nullptr;
nsAlertsIconListener::notify_notification_new_t nsAlertsIconListener::notify_notification_new = nullptr;
nsAlertsIconListener::notify_notification_show_t nsAlertsIconListener::notify_notification_show = nullptr;
nsAlertsIconListener::notify_notification_set_icon_from_pixbuf_t nsAlertsIconListener::notify_notification_set_icon_from_pixbuf = nullptr;
nsAlertsIconListener::notify_notification_add_action_t nsAlertsIconListener::notify_notification_add_action = nullptr;

static void notify_action_cb(NotifyNotification *notification,
                             gchar *action, gpointer user_data)
{
  nsAlertsIconListener* alert = static_cast<nsAlertsIconListener*> (user_data);
  alert->SendCallback();
}

static void notify_closed_marshal(GClosure* closure,
                                  GValue* return_value,
                                  guint n_param_values,
                                  const GValue* param_values,
                                  gpointer invocation_hint,
                                  gpointer marshal_data)
{
  MOZ_ASSERT(n_param_values >= 1, "No object in params");

  nsAlertsIconListener* alert =
    static_cast<nsAlertsIconListener*>(closure->data);
  alert->SendClosed();
  NS_RELEASE(alert);
}

static GdkPixbuf*
GetPixbufFromImgRequest(imgIRequest* aRequest)
{
  nsCOMPtr<imgIContainer> image;
  nsresult rv = aRequest->GetImage(getter_AddRefs(image));
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  nsCOMPtr<nsIImageToPixbuf> imgToPixbuf =
    do_GetService("@mozilla.org/widget/image-to-gdk-pixbuf;1");

  return imgToPixbuf->ConvertImageToPixbuf(image);
}

NS_IMPL_ISUPPORTS(nsAlertsIconListener, imgINotificationObserver,
                  nsIObserver, nsISupportsWeakReference)

nsAlertsIconListener::nsAlertsIconListener()
: mLoadedFrame(false),
  mNotification(nullptr)
{
  if (!libNotifyHandle && !libNotifyNotAvail) {
    libNotifyHandle = dlopen("libnotify.so.4", RTLD_LAZY);
    if (!libNotifyHandle) {
      libNotifyHandle = dlopen("libnotify.so.1", RTLD_LAZY);
      if (!libNotifyHandle) {
        libNotifyNotAvail = true;
        return;
      }
    }

    notify_is_initted = (notify_is_initted_t)dlsym(libNotifyHandle, "notify_is_initted");
    notify_init = (notify_init_t)dlsym(libNotifyHandle, "notify_init");
    notify_get_server_caps = (notify_get_server_caps_t)dlsym(libNotifyHandle, "notify_get_server_caps");
    notify_notification_new = (notify_notification_new_t)dlsym(libNotifyHandle, "notify_notification_new");
    notify_notification_show = (notify_notification_show_t)dlsym(libNotifyHandle, "notify_notification_show");
    notify_notification_set_icon_from_pixbuf = (notify_notification_set_icon_from_pixbuf_t)dlsym(libNotifyHandle, "notify_notification_set_icon_from_pixbuf");
    notify_notification_add_action = (notify_notification_add_action_t)dlsym(libNotifyHandle, "notify_notification_add_action");
    if (!notify_is_initted || !notify_init || !notify_get_server_caps || !notify_notification_new || !notify_notification_show || !notify_notification_set_icon_from_pixbuf || !notify_notification_add_action) {
      dlclose(libNotifyHandle);
      libNotifyHandle = nullptr;
    }
  }
}

nsAlertsIconListener::~nsAlertsIconListener()
{
  if (mIconRequest)
    mIconRequest->CancelAndForgetObserver(NS_BINDING_ABORTED);
  
}

NS_IMETHODIMP
nsAlertsIconListener::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    return OnLoadComplete(aRequest);
  }

  if (aType == imgINotificationObserver::FRAME_COMPLETE) {
    return OnFrameComplete(aRequest);
  }

  return NS_OK;
}

nsresult
nsAlertsIconListener::OnLoadComplete(imgIRequest* aRequest)
{
  NS_ASSERTION(mIconRequest == aRequest, "aRequest does not match!");

  uint32_t imgStatus = imgIRequest::STATUS_ERROR;
  nsresult rv = aRequest->GetImageStatus(&imgStatus);
  NS_ENSURE_SUCCESS(rv, rv);
  if ((imgStatus & imgIRequest::STATUS_ERROR) && !mLoadedFrame) {
    
    ShowAlert(nullptr);

    
    mIconRequest->Cancel(NS_BINDING_ABORTED);
    mIconRequest = nullptr;
  }

  nsCOMPtr<imgIContainer> image;
  rv = aRequest->GetImage(getter_AddRefs(image));
  if (NS_WARN_IF(NS_FAILED(rv) || !image)) {
    return rv;
  }

  
  int32_t width = 0, height = 0;
  image->GetWidth(&width);
  image->GetHeight(&height);
  image->RequestDecodeForSize(nsIntSize(width, height), imgIContainer::FLAG_NONE);

  return NS_OK;
}

nsresult
nsAlertsIconListener::OnFrameComplete(imgIRequest* aRequest)
{
  NS_ASSERTION(mIconRequest == aRequest, "aRequest does not match!");

  if (mLoadedFrame)
    return NS_OK; 

  GdkPixbuf* imagePixbuf = GetPixbufFromImgRequest(aRequest);
  if (!imagePixbuf) {
    ShowAlert(nullptr);
  } else {
    ShowAlert(imagePixbuf);
    g_object_unref(imagePixbuf);
  }

  mLoadedFrame = true;

  
  mIconRequest->Cancel(NS_BINDING_ABORTED);
  mIconRequest = nullptr;

  return NS_OK;
}

nsresult
nsAlertsIconListener::ShowAlert(GdkPixbuf* aPixbuf)
{
  mNotification = notify_notification_new(mAlertTitle.get(), mAlertText.get(),
                                          nullptr, nullptr);

  if (!mNotification)
    return NS_ERROR_OUT_OF_MEMORY;

  if (aPixbuf)
    notify_notification_set_icon_from_pixbuf(mNotification, aPixbuf);

  NS_ADDREF(this);
  if (mAlertHasAction) {
    
    
    
    notify_notification_add_action(mNotification, "default", "Activate",
                                   notify_action_cb, this, nullptr);
  }

  
  
  
  
  GClosure* closure = g_closure_new_simple(sizeof(GClosure), this);
  g_closure_set_marshal(closure, notify_closed_marshal);
  mClosureHandler = g_signal_connect_closure(mNotification, "closed", closure, FALSE);
  gboolean result = notify_notification_show(mNotification, nullptr);

  if (result && mAlertListener)
    mAlertListener->Observe(nullptr, "alertshow", mAlertCookie.get());

  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsAlertsIconListener::StartRequest(const nsAString & aImageUrl, bool aInPrivateBrowsing)
{
  if (mIconRequest) {
    
    mIconRequest->Cancel(NS_BINDING_ABORTED);
    mIconRequest = nullptr;
  }

  nsCOMPtr<nsIURI> imageUri;
  NS_NewURI(getter_AddRefs(imageUri), aImageUrl);
  if (!imageUri)
    return ShowAlert(nullptr);

  nsCOMPtr<imgILoader> il(do_GetService("@mozilla.org/image/loader;1"));
  if (!il)
    return ShowAlert(nullptr);

  nsresult rv = il->LoadImageXPCOM(imageUri, nullptr, nullptr,
                                   NS_LITERAL_STRING("default"), nullptr, nullptr,
                                   this, nullptr,
                                   aInPrivateBrowsing ? nsIRequest::LOAD_ANONYMOUS :
                                                        nsIRequest::LOAD_NORMAL,
                                   nullptr, 0 ,
                                   getter_AddRefs(mIconRequest));
  if (NS_FAILED(rv))
    return rv;

  return NS_OK;
}

void
nsAlertsIconListener::SendCallback()
{
  if (mAlertListener)
    mAlertListener->Observe(nullptr, "alertclickcallback", mAlertCookie.get());
}

void
nsAlertsIconListener::SendClosed()
{
  if (mNotification) {
    g_object_unref(mNotification);
    mNotification = nullptr;
  }
  if (mAlertListener)
    mAlertListener->Observe(nullptr, "alertfinished", mAlertCookie.get());
}

NS_IMETHODIMP
nsAlertsIconListener::Observe(nsISupports *aSubject, const char *aTopic,
                              const char16_t *aData) {
  
  
  if (!nsCRT::strcmp(aTopic, "quit-application") && mNotification) {
    g_signal_handler_disconnect(mNotification, mClosureHandler);
    g_object_unref(mNotification);
    mNotification = nullptr;
    Release(); 
  }
  return NS_OK;
}

nsresult
nsAlertsIconListener::InitAlertAsync(const nsAString & aImageUrl,
                                     const nsAString & aAlertTitle, 
                                     const nsAString & aAlertText,
                                     bool aAlertTextClickable,
                                     const nsAString & aAlertCookie,
                                     nsIObserver * aAlertListener,
                                     bool aInPrivateBrowsing)
{
  if (!libNotifyHandle)
    return NS_ERROR_FAILURE;

  if (!notify_is_initted()) {
    
    nsCOMPtr<nsIStringBundleService> bundleService = 
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);

    nsAutoCString appShortName;
    if (bundleService) {
      nsCOMPtr<nsIStringBundle> bundle;
      bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                                  getter_AddRefs(bundle));
      nsAutoString appName;

      if (bundle) {
        bundle->GetStringFromName(MOZ_UTF16("brandShortName"),
                                  getter_Copies(appName));
        appShortName = NS_ConvertUTF16toUTF8(appName);
      } else {
        NS_WARNING("brand.properties not present, using default application name");
        appShortName.AssignLiteral("Mozilla");
      }
    } else {
      appShortName.AssignLiteral("Mozilla");
    }

    if (!notify_init(appShortName.get()))
      return NS_ERROR_FAILURE;

    GList *server_caps = notify_get_server_caps();
    if (server_caps) {
      gHasCaps = true;
      for (GList* cap = server_caps; cap != nullptr; cap = cap->next) {
        if (!strcmp((char*) cap->data, "actions")) {
          gHasActions = true;
          break;
        }
      }
      g_list_foreach(server_caps, (GFunc)g_free, nullptr);
      g_list_free(server_caps);
    }
  }

  if (!gHasCaps) {
    
    
    return NS_ERROR_FAILURE;
  }

  if (!gHasActions && aAlertTextClickable)
    return NS_ERROR_FAILURE; 

  nsCOMPtr<nsIObserverService> obsServ =
      do_GetService("@mozilla.org/observer-service;1");
  if (obsServ)
    obsServ->AddObserver(this, "quit-application", true);

  
  
  if (aAlertTitle.IsEmpty()) {
    mAlertTitle = NS_LITERAL_CSTRING(" ");
  } else {
    mAlertTitle = NS_ConvertUTF16toUTF8(aAlertTitle);
  }

  mAlertText = NS_ConvertUTF16toUTF8(aAlertText);
  mAlertHasAction = aAlertTextClickable;

  mAlertListener = aAlertListener;
  mAlertCookie = aAlertCookie;

  return StartRequest(aImageUrl, aInPrivateBrowsing);
}
