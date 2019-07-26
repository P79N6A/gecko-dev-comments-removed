




#include "mozilla/Hal.h"
#include "nsScreen.h"
#include "nsIDocShell.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfoID.h"
#include "nsIDocShellTreeItem.h"
#include "nsLayoutUtils.h"
#include "nsDOMEvent.h"
#include "nsGlobalWindow.h"
#include "nsJSUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace {

bool
IsChromeType(nsIDocShell *aDocShell)
{
  nsCOMPtr<nsIDocShellTreeItem> ds = do_QueryInterface(aDocShell);
  if (!ds) {
    return false;
  }

  int32_t itemType;
  ds->GetItemType(&itemType);
  return itemType == nsIDocShellTreeItem::typeChrome;
}

} 

 already_AddRefed<nsScreen>
nsScreen::Create(nsPIDOMWindow* aWindow)
{
  MOZ_ASSERT(aWindow);

  if (!aWindow->GetDocShell()) {
    return nullptr;
  }

  nsCOMPtr<nsIScriptGlobalObject> sgo =
    do_QueryInterface(static_cast<nsPIDOMWindow*>(aWindow));
  NS_ENSURE_TRUE(sgo, nullptr);

  nsRefPtr<nsScreen> screen = new nsScreen();
  screen->BindToOwner(aWindow);

  hal::RegisterScreenConfigurationObserver(screen);
  hal::ScreenConfiguration config;
  hal::GetCurrentScreenConfiguration(&config);
  screen->mOrientation = config.orientation();

  return screen.forget();
}

nsScreen::nsScreen()
  : mEventListener(nullptr)
{
}

void
nsScreen::Reset()
{
  hal::UnlockScreenOrientation();

  if (mEventListener) {
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(GetOwner());
    if (target) {
      target->RemoveSystemEventListener(NS_LITERAL_STRING("mozfullscreenchange"),
                                        mEventListener,  true);
    }

    mEventListener = nullptr;
  }
}

nsScreen::~nsScreen()
{
  Reset();
  hal::UnregisterScreenConfigurationObserver(this);
}


DOMCI_DATA(Screen, nsScreen)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsScreen)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsScreen,
                                                  nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsScreen,
                                                nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsScreen)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScreen)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMScreen)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Screen)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsScreen, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsScreen, nsDOMEventTargetHelper)

NS_IMPL_EVENT_HANDLER(nsScreen, mozorientationchange)

NS_IMETHODIMP
nsScreen::GetTop(int32_t* aTop)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aTop = rect.y;

  return rv;
}


NS_IMETHODIMP
nsScreen::GetLeft(int32_t* aLeft)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aLeft = rect.x;

  return rv;
}


NS_IMETHODIMP
nsScreen::GetWidth(int32_t* aWidth)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aWidth = rect.width;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetHeight(int32_t* aHeight)
{
  nsRect rect;
  nsresult rv = GetRect(rect);

  *aHeight = rect.height;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetPixelDepth(int32_t* aPixelDepth)
{
  nsDeviceContext* context = GetDeviceContext();

  if (!context) {
    *aPixelDepth = -1;

    return NS_ERROR_FAILURE;
  }

  uint32_t depth;
  context->GetDepth(depth);

  *aPixelDepth = depth;

  return NS_OK;
}

NS_IMETHODIMP
nsScreen::GetColorDepth(int32_t* aColorDepth)
{
  return GetPixelDepth(aColorDepth);
}

NS_IMETHODIMP
nsScreen::GetAvailWidth(int32_t* aAvailWidth)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailWidth = rect.width;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetAvailHeight(int32_t* aAvailHeight)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailHeight = rect.height;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetAvailLeft(int32_t* aAvailLeft)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailLeft = rect.x;

  return rv;
}

NS_IMETHODIMP
nsScreen::GetAvailTop(int32_t* aAvailTop)
{
  nsRect rect;
  nsresult rv = GetAvailRect(rect);

  *aAvailTop = rect.y;

  return rv;
}

nsDeviceContext*
nsScreen::GetDeviceContext()
{
  return nsLayoutUtils::GetDeviceContextForScreenInfo(GetOwner());
}

nsresult
nsScreen::GetRect(nsRect& aRect)
{
  nsDeviceContext *context = GetDeviceContext();

  if (!context) {
    return NS_ERROR_FAILURE;
  }

  context->GetRect(aRect);

  aRect.x = nsPresContext::AppUnitsToIntCSSPixels(aRect.x);
  aRect.y = nsPresContext::AppUnitsToIntCSSPixels(aRect.y);
  aRect.height = nsPresContext::AppUnitsToIntCSSPixels(aRect.height);
  aRect.width = nsPresContext::AppUnitsToIntCSSPixels(aRect.width);

  return NS_OK;
}

nsresult
nsScreen::GetAvailRect(nsRect& aRect)
{
  nsDeviceContext *context = GetDeviceContext();

  if (!context) {
    return NS_ERROR_FAILURE;
  }

  context->GetClientRect(aRect);

  aRect.x = nsPresContext::AppUnitsToIntCSSPixels(aRect.x);
  aRect.y = nsPresContext::AppUnitsToIntCSSPixels(aRect.y);
  aRect.height = nsPresContext::AppUnitsToIntCSSPixels(aRect.height);
  aRect.width = nsPresContext::AppUnitsToIntCSSPixels(aRect.width);

  return NS_OK;
}

void
nsScreen::Notify(const hal::ScreenConfiguration& aConfiguration)
{
  ScreenOrientation previousOrientation = mOrientation;
  mOrientation = aConfiguration.orientation();

  NS_ASSERTION(mOrientation == eScreenOrientation_PortraitPrimary ||
               mOrientation == eScreenOrientation_PortraitSecondary ||
               mOrientation == eScreenOrientation_LandscapePrimary ||
               mOrientation == eScreenOrientation_LandscapeSecondary,
               "Invalid orientation value passed to notify method!");

  if (mOrientation != previousOrientation) {
    DispatchTrustedEvent(NS_LITERAL_STRING("mozorientationchange"));
  }
}

NS_IMETHODIMP
nsScreen::GetMozOrientation(nsAString& aOrientation)
{
  switch (mOrientation) {
    case eScreenOrientation_PortraitPrimary:
      aOrientation.AssignLiteral("portrait-primary");
      break;
    case eScreenOrientation_PortraitSecondary:
      aOrientation.AssignLiteral("portrait-secondary");
      break;
    case eScreenOrientation_LandscapePrimary:
      aOrientation.AssignLiteral("landscape-primary");
      break;
    case eScreenOrientation_LandscapeSecondary:
      aOrientation.AssignLiteral("landscape-secondary");
      break;
    case eScreenOrientation_None:
    default:
      MOZ_ASSERT(false);
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsScreen::LockPermission
nsScreen::GetLockOrientationPermission() const
{
  nsCOMPtr<nsPIDOMWindow> owner = GetOwner();
  if (!owner) {
    return LOCK_DENIED;
  }

  
  if (IsChromeType(owner->GetDocShell())) {
    return LOCK_ALLOWED;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  owner->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (!doc) {
    return LOCK_DENIED;
  }

  
  if (doc->NodePrincipal()->GetAppStatus() >=
        nsIPrincipal::APP_STATUS_INSTALLED) {
    return LOCK_ALLOWED;
  }

  
  bool fullscreen;
  domDoc->GetMozFullScreen(&fullscreen);

  return fullscreen ? FULLSCREEN_LOCK_ALLOWED : LOCK_DENIED;
}

NS_IMETHODIMP
nsScreen::MozLockOrientation(const JS::Value& aOrientation, JSContext* aCx,
                             bool* aReturn)
{
  if (aOrientation.isObject() && IsArrayLike(aCx, &aOrientation.toObject())) {
    JSObject* seq = &aOrientation.toObject();
    uint32_t length;
    
    if (!JS_GetArrayLength(aCx, seq, &length)) {
      return NS_ERROR_FAILURE;
    }

    Sequence<nsString> orientations;
    if (!orientations.SetCapacity(length)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    for (uint32_t i = 0; i < length; ++i) {
      JS::Value temp;
      if (!JS_GetElement(aCx, seq, i, &temp)) {
        return NS_ERROR_FAILURE;
      }

      js::RootedString jsString(aCx, JS_ValueToString(aCx, temp));
      if (!jsString) {
        return NS_ERROR_FAILURE;
      }

      nsDependentJSString str;
      if (!str.init(aCx, jsString)) {
        return NS_ERROR_FAILURE;
      }

      *orientations.AppendElement() = str;
    }

    ErrorResult rv;
    *aReturn = MozLockOrientation(orientations, rv);
    return rv.ErrorCode();
  }

  js::RootedString jsString(aCx, JS_ValueToString(aCx, aOrientation));
  if (!jsString) {
    return NS_ERROR_FAILURE;
  }

  nsDependentJSString orientation;
  if (!orientation.init(aCx, jsString)) {
    return NS_ERROR_FAILURE;
  }

  ErrorResult rv;
  *aReturn = MozLockOrientation(orientation, rv);
  return rv.ErrorCode();
}

bool
nsScreen::MozLockOrientation(const nsAString& aOrientation, ErrorResult& aRv)
{
  nsString orientation(aOrientation);
  Sequence<nsString> orientations;
  if (!orientations.AppendElement(orientation)) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return false;
  }
  return MozLockOrientation(orientations, aRv);
}

bool
nsScreen::MozLockOrientation(const Sequence<nsString>& aOrientations,
                             ErrorResult& aRv)
{
  ScreenOrientation orientation = eScreenOrientation_None;

  for (uint32_t i = 0; i < aOrientations.Length(); ++i) {
    const nsString& item = aOrientations[i];

    if (item.EqualsLiteral("portrait")) {
      orientation |= eScreenOrientation_PortraitPrimary |
                     eScreenOrientation_PortraitSecondary;
    } else if (item.EqualsLiteral("portrait-primary")) {
      orientation |= eScreenOrientation_PortraitPrimary;
    } else if (item.EqualsLiteral("portrait-secondary")) {
      orientation |= eScreenOrientation_PortraitSecondary;
    } else if (item.EqualsLiteral("landscape")) {
      orientation |= eScreenOrientation_LandscapePrimary |
                     eScreenOrientation_LandscapeSecondary;
    } else if (item.EqualsLiteral("landscape-primary")) {
      orientation |= eScreenOrientation_LandscapePrimary;
    } else if (item.EqualsLiteral("landscape-secondary")) {
      orientation |= eScreenOrientation_LandscapeSecondary;
    } else {
      
      
      return false;
    }
  }

  switch (GetLockOrientationPermission()) {
    case LOCK_DENIED:
      return false;
    case LOCK_ALLOWED:
      return hal::LockScreenOrientation(orientation);
    case FULLSCREEN_LOCK_ALLOWED: {
      if (!hal::LockScreenOrientation(orientation)) {
        return false;
      }

      
      
      
      nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(GetOwner());
      if (!target) {
        
        return true;
      }

      if (!mEventListener) {
        mEventListener = new FullScreenEventListener();
      }

      aRv = target->AddSystemEventListener(NS_LITERAL_STRING("mozfullscreenchange"),
                                           mEventListener,  true);
      return true;
    }
  }

  
  
  MOZ_NOT_REACHED();
  return false;
}

NS_IMETHODIMP
nsScreen::MozUnlockOrientation()
{
  hal::UnlockScreenOrientation();
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsScreen::FullScreenEventListener, nsIDOMEventListener)

NS_IMETHODIMP
nsScreen::FullScreenEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
#ifdef DEBUG
  nsAutoString eventType;
  aEvent->GetType(eventType);

  MOZ_ASSERT(eventType.EqualsLiteral("mozfullscreenchange"));
#endif

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetCurrentTarget(getter_AddRefs(target));

  
  
  
  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(target);
  MOZ_ASSERT(window);

  nsCOMPtr<nsIDOMDocument> doc;
  window->GetDocument(getter_AddRefs(doc));
  
  
  if (doc) {
    bool fullscreen;
    doc->GetMozFullScreen(&fullscreen);
    if (fullscreen) {
      return NS_OK;
    }
  }

  target->RemoveSystemEventListener(NS_LITERAL_STRING("mozfullscreenchange"),
                                    this, true);

  hal::UnlockScreenOrientation();

  return NS_OK;
}
