






#include "ApplicationAccessible.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "Relation.h"
#include "Role.h"
#include "States.h"

#include "nsIComponentManager.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIWindowMediator.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"
#include "nsIStringBundle.h"

using namespace mozilla::a11y;

ApplicationAccessible::ApplicationAccessible() :
  AccessibleWrap(nullptr, nullptr)
{
  mType = eApplicationType;
  mAppInfo = do_GetService("@mozilla.org/xre/app-info;1");
}




NS_IMPL_ISUPPORTS_INHERITED(ApplicationAccessible, Accessible,
                            nsIAccessibleApplication)




ENameValueFlag
ApplicationAccessible::Name(nsString& aName)
{
  aName.Truncate();

  nsCOMPtr<nsIStringBundleService> bundleService =
    mozilla::services::GetStringBundleService();

  NS_ASSERTION(bundleService, "String bundle service must be present!");
  if (!bundleService)
    return eNameOK;

  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                                            getter_AddRefs(bundle));
  if (NS_FAILED(rv))
    return eNameOK;

  nsXPIDLString appName;
  rv = bundle->GetStringFromName(MOZ_UTF16("brandShortName"),
                                 getter_Copies(appName));
  if (NS_FAILED(rv) || appName.IsEmpty()) {
    NS_WARNING("brandShortName not found, using default app name");
    appName.AssignLiteral("Gecko based application");
  }

  aName.Assign(appName);
  return eNameOK;
}

void
ApplicationAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();
}

void
ApplicationAccessible::Value(nsString& aValue)
{
  aValue.Truncate();
}

uint64_t
ApplicationAccessible::State()
{
  return IsDefunct() ? states::DEFUNCT : 0;
}

already_AddRefed<nsIPersistentProperties>
ApplicationAccessible::NativeAttributes()
{
  return nullptr;
}

GroupPos
ApplicationAccessible::GroupPosition()
{
  return GroupPos();
}

Accessible*
ApplicationAccessible::ChildAtPoint(int32_t aX, int32_t aY,
                                    EWhichChildAtPoint aWhichChild)
{
  return nullptr;
}

Accessible*
ApplicationAccessible::FocusedChild()
{
  Accessible* focus = FocusMgr()->FocusedAccessible();
  if (focus && focus->Parent() == this)
    return focus;

  return nullptr;
}

Relation
ApplicationAccessible::RelationByType(RelationType aRelationType)
{
  return Relation();
}

nsIntRect
ApplicationAccessible::Bounds() const
{
  return nsIntRect();
}




NS_IMETHODIMP
ApplicationAccessible::GetAppName(nsAString& aName)
{
  aName.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsAutoCString cname;
  nsresult rv = mAppInfo->GetName(cname);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cname, aName);
  return NS_OK;
}

NS_IMETHODIMP
ApplicationAccessible::GetAppVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsAutoCString cversion;
  nsresult rv = mAppInfo->GetVersion(cversion);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cversion, aVersion);
  return NS_OK;
}

NS_IMETHODIMP
ApplicationAccessible::GetPlatformName(nsAString& aName)
{
  aName.AssignLiteral("Gecko");
  return NS_OK;
}

NS_IMETHODIMP
ApplicationAccessible::GetPlatformVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsAutoCString cversion;
  nsresult rv = mAppInfo->GetPlatformVersion(cversion);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cversion, aVersion);
  return NS_OK;
}




void
ApplicationAccessible::Shutdown()
{
  mAppInfo = nullptr;
}

void
ApplicationAccessible::ApplyARIAState(uint64_t* aState) const
{
}

role
ApplicationAccessible::NativeRole()
{
  return roles::APP_ROOT;
}

uint64_t
ApplicationAccessible::NativeState()
{
  return 0;
}

void
ApplicationAccessible::InvalidateChildren()
{
  
  
}

KeyBinding
ApplicationAccessible::AccessKey() const
{
  return KeyBinding();
}




void
ApplicationAccessible::CacheChildren()
{
  
  
  

  
  
  
  
  

  nsCOMPtr<nsIWindowMediator> windowMediator =
    do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  nsresult rv = windowMediator->GetEnumerator(nullptr,
                                              getter_AddRefs(windowEnumerator));
  if (NS_FAILED(rv))
    return;

  bool hasMore = false;
  windowEnumerator->HasMoreElements(&hasMore);
  while (hasMore) {
    nsCOMPtr<nsISupports> window;
    windowEnumerator->GetNext(getter_AddRefs(window));
    nsCOMPtr<nsIDOMWindow> DOMWindow = do_QueryInterface(window);
    if (DOMWindow) {
      nsCOMPtr<nsIDOMDocument> DOMDocument;
      DOMWindow->GetDocument(getter_AddRefs(DOMDocument));
      if (DOMDocument) {
        nsCOMPtr<nsIDocument> docNode(do_QueryInterface(DOMDocument));
        GetAccService()->GetDocAccessible(docNode); 
      }
    }
    windowEnumerator->HasMoreElements(&hasMore);
  }
}

Accessible*
ApplicationAccessible::GetSiblingAtOffset(int32_t aOffset,
                                          nsresult* aError) const
{
  if (aError)
    *aError = NS_OK; 

  return nullptr;
}
