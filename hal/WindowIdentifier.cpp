





#include "mozilla/dom/ContentChild.h"
#include "WindowIdentifier.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace hal {

WindowIdentifier::WindowIdentifier()
  : mWindow(nullptr)
  , mIsEmpty(true)
{
}

WindowIdentifier::WindowIdentifier(nsIDOMWindow *window)
  : mWindow(window)
  , mIsEmpty(false)
{
  mID.AppendElement(GetWindowID());
}

WindowIdentifier::WindowIdentifier(const InfallibleTArray<uint64_t> &id, nsIDOMWindow *window)
  : mID(id)
  , mWindow(window)
  , mIsEmpty(false)
{
  mID.AppendElement(GetWindowID());
}

WindowIdentifier::WindowIdentifier(const WindowIdentifier &other)
  : mID(other.mID)
  , mWindow(other.mWindow)
  , mIsEmpty(other.mIsEmpty)
{
}

const InfallibleTArray<uint64_t>&
WindowIdentifier::AsArray() const
{
  MOZ_ASSERT(!mIsEmpty);
  return mID;
}

bool
WindowIdentifier::HasTraveledThroughIPC() const
{
  MOZ_ASSERT(!mIsEmpty);
  return mID.Length() >= 2;
}

void
WindowIdentifier::AppendProcessID()
{
  MOZ_ASSERT(!mIsEmpty);
  mID.AppendElement(dom::ContentChild::GetSingleton()->GetID());
}

uint64_t
WindowIdentifier::GetWindowID() const
{
  MOZ_ASSERT(!mIsEmpty);
  nsCOMPtr<nsPIDOMWindow> pidomWindow = do_QueryInterface(mWindow);
  if (!pidomWindow) {
    return UINT64_MAX;
  }
  return pidomWindow->WindowID();
}

nsIDOMWindow*
WindowIdentifier::GetWindow() const
{
  MOZ_ASSERT(!mIsEmpty);
  return mWindow;
}

} 
} 
