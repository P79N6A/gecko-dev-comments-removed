





#include "MediaStreamError.h"
#include "mozilla/dom/MediaStreamErrorBinding.h"
#include "nsContentUtils.h"

namespace mozilla {

BaseMediaMgrError::BaseMediaMgrError(const nsAString& aName,
                                     const nsAString& aMessage,
                                     const nsAString& aConstraintName)
  : mName(aName)
  , mMessage(aMessage)
  , mConstraintName(aConstraintName)
{
  if (mMessage.IsEmpty()) {
    if (mName.EqualsLiteral("NotFoundError")) {
      mMessage.AssignLiteral("The object can not be found here.");
    } else if (mName.EqualsLiteral("PermissionDeniedError")) {
      mMessage.AssignLiteral("The user did not grant permission for the operation.");
    } else if (mName.EqualsLiteral("SourceUnavailableError")) {
      mMessage.AssignLiteral("The source of the MediaStream could not be "
          "accessed due to a hardware error (e.g. lock from another process).");
    } else if (mName.EqualsLiteral("InternalError")) {
      mMessage.AssignLiteral("Internal error.");
    }
  }
}


NS_IMPL_ISUPPORTS0(MediaMgrError)

namespace dom {

MediaStreamError::MediaStreamError(
    nsPIDOMWindow* aParent,
    const nsAString& aName,
    const nsAString& aMessage,
    const nsAString& aConstraintName)
  : BaseMediaMgrError(aName, aMessage, aConstraintName)
  , mParent(aParent) {}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MediaStreamError, mParent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(MediaStreamError)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MediaStreamError)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MediaStreamError)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
MediaStreamError::WrapObject(JSContext* aCx)
{
  return MediaStreamErrorBinding::Wrap(aCx, this);
}

void
MediaStreamError::GetName(nsAString& aName) const
{
  aName = mName;
}

void
MediaStreamError::GetMessage(nsAString& aMessage) const
{
  aMessage = mMessage;
}

void
MediaStreamError::GetConstraintName(nsAString& aConstraintName) const
{
  aConstraintName = mConstraintName;
}

} 
} 
