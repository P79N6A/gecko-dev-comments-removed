





#include "nsBrowserElement.h"

#include "mozilla/dom/BrowserElementBinding.h"
#include "mozilla/dom/DOMRequest.h"
#include "mozilla/Preferences.h"

namespace mozilla {

void
nsBrowserElement::SetVisible(bool aVisible, ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::GetVisible(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

void
nsBrowserElement::SetActive(bool aVisible, ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

bool
nsBrowserElement::GetActive(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return false;
}

void
nsBrowserElement::SendMouseEvent(const nsAString& aType,
                                 uint32_t aX,
                                 uint32_t aY,
                                 uint32_t aButton,
                                 uint32_t aClickCount,
                                 uint32_t aModifiers,
                                 ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

void
nsBrowserElement::SendTouchEvent(const nsAString& aType,
                                 const dom::Sequence<uint32_t>& aIdentifiers,
                                 const dom::Sequence<int32_t>& aXs,
                                 const dom::Sequence<int32_t>& aYs,
                                 const dom::Sequence<uint32_t>& aRxs,
                                 const dom::Sequence<uint32_t>& aRys,
                                 const dom::Sequence<float>& aRotationAngles,
                                 const dom::Sequence<float>& aForces,
                                 uint32_t aCount,
                                 uint32_t aModifiers,
                                 ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

void
nsBrowserElement::GoBack(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

void
nsBrowserElement::GoForward(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

void
nsBrowserElement::Reload(bool aHardReload, ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

void
nsBrowserElement::Stop(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::Download(const nsAString& aUrl,
                           const dom::BrowserElementDownloadOptions& aOptions,
                           ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::PurgeHistory(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::GetScreenshot(uint32_t aWidth,
                                uint32_t aHeight,
                                const dom::Optional<nsAString>& aMimeType,
                                ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

void
nsBrowserElement::Zoom(float aZoom, ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::GetCanGoBack(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::GetCanGoForward(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::GetContentDimensions(ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

void
nsBrowserElement::AddNextPaintListener(dom::BrowserElementNextPaintEventCallback& aListener,
                                       ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

void
nsBrowserElement::RemoveNextPaintListener(dom::BrowserElementNextPaintEventCallback& aListener,
                                          ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}

already_AddRefed<dom::DOMRequest>
nsBrowserElement::SetInputMethodActive(bool aIsActive,
                                       ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

} 
