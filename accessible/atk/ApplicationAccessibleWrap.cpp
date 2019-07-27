





#include "ApplicationAccessibleWrap.h"

#include "nsCOMPtr.h"
#include "nsMai.h"
#include "nsAutoPtr.h"
#include "nsAccessibilityService.h"

#include <gtk/gtk.h>
#include <atk/atk.h>

using namespace mozilla;
using namespace mozilla::a11y;




ApplicationAccessibleWrap::ApplicationAccessibleWrap():
  ApplicationAccessible()
{
}

ApplicationAccessibleWrap::~ApplicationAccessibleWrap()
{
  AccessibleWrap::ShutdownAtkObject();
}

gboolean
toplevel_event_watcher(GSignalInvocationHint* ihint,
                       guint                  n_param_values,
                       const GValue*          param_values,
                       gpointer               data)
{
  static GQuark sQuark_gecko_acc_obj = 0;

  if (!sQuark_gecko_acc_obj)
    sQuark_gecko_acc_obj = g_quark_from_static_string("GeckoAccObj");

  if (nsAccessibilityService::IsShutdown())
    return TRUE;

  GObject* object = reinterpret_cast<GObject*>(g_value_get_object(param_values));
  if (!GTK_IS_WINDOW(object))
    return TRUE;

  AtkObject* child = gtk_widget_get_accessible(GTK_WIDGET(object));

  
  if (!IS_MAI_OBJECT(child) &&
      (atk_object_get_role(child) == ATK_ROLE_DIALOG)) {

    if (data == reinterpret_cast<gpointer>(nsIAccessibleEvent::EVENT_SHOW)) {

      
      Accessible* windowAcc = GetAccService()->AddNativeRootAccessible(child);
      g_object_set_qdata(G_OBJECT(child), sQuark_gecko_acc_obj,
                         reinterpret_cast<gpointer>(windowAcc));

    } else {

      
      Accessible* windowAcc =
        reinterpret_cast<Accessible*>
                        (g_object_get_qdata(G_OBJECT(child), sQuark_gecko_acc_obj));
      if (windowAcc) {
        GetAccService()->RemoveNativeRootAccessible(windowAcc);
        g_object_set_qdata(G_OBJECT(child), sQuark_gecko_acc_obj, nullptr);
      }

    }
  }

  return TRUE;
}

ENameValueFlag
ApplicationAccessibleWrap::Name(nsString& aName)
{
  
  
  
  
  AppName(aName);
  return eNameOK;
}

NS_IMETHODIMP
ApplicationAccessibleWrap::GetNativeInterface(void** aOutAccessible)
{
    *aOutAccessible = nullptr;

    if (!mAtkObject) {
        mAtkObject =
            reinterpret_cast<AtkObject *>
                            (g_object_new(MAI_TYPE_ATK_OBJECT, nullptr));
        NS_ENSURE_TRUE(mAtkObject, NS_ERROR_OUT_OF_MEMORY);

        atk_object_initialize(mAtkObject, this);
        mAtkObject->role = ATK_ROLE_INVALID;
        mAtkObject->layer = ATK_LAYER_INVALID;
    }

    *aOutAccessible = mAtkObject;
    return NS_OK;
}

struct AtkRootAccessibleAddedEvent {
  AtkObject *app_accessible;
  AtkObject *root_accessible;
  uint32_t index;
};

gboolean fireRootAccessibleAddedCB(gpointer data)
{
    AtkRootAccessibleAddedEvent* eventData = (AtkRootAccessibleAddedEvent*)data;
    g_signal_emit_by_name(eventData->app_accessible, "children_changed::add",
                          eventData->index, eventData->root_accessible, nullptr);
    g_object_unref(eventData->app_accessible);
    g_object_unref(eventData->root_accessible);
    free(data);

    return FALSE;
}

bool
ApplicationAccessibleWrap::InsertChildAt(uint32_t aIdx, Accessible* aChild)
{
  if (!ApplicationAccessible::InsertChildAt(aIdx, aChild))
    return false;

  AtkObject* atkAccessible = AccessibleWrap::GetAtkObject(aChild);
  atk_object_set_parent(atkAccessible, mAtkObject);

    uint32_t count = mChildren.Length();

    
    
    AtkRootAccessibleAddedEvent* eventData = (AtkRootAccessibleAddedEvent*)
      malloc(sizeof(AtkRootAccessibleAddedEvent));
    if (eventData) {
      eventData->app_accessible = mAtkObject;
      eventData->root_accessible = atkAccessible;
      eventData->index = count -1;
      g_object_ref(mAtkObject);
      g_object_ref(atkAccessible);
      g_timeout_add(0, fireRootAccessibleAddedCB, eventData);
    }

    return true;
}

bool
ApplicationAccessibleWrap::RemoveChild(Accessible* aChild)
{
  int32_t index = aChild->IndexInParent();

  AtkObject* atkAccessible = AccessibleWrap::GetAtkObject(aChild);
  atk_object_set_parent(atkAccessible, nullptr);
  g_signal_emit_by_name(mAtkObject, "children_changed::remove", index,
                        atkAccessible, nullptr);

  return ApplicationAccessible::RemoveChild(aChild);
}

