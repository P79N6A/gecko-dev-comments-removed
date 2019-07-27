





#include "DetailedPromise.h"
#include "mozilla/dom/DOMException.h"

namespace mozilla {
namespace dom {

DetailedPromise::DetailedPromise(nsIGlobalObject* aGlobal)
  : Promise(aGlobal)
  , mResponded(false)
{
}

DetailedPromise::~DetailedPromise()
{
  MOZ_ASSERT(mResponded == IsPending());
}

static void
LogToConsole(const nsAString& aMsg)
{
  nsCOMPtr<nsIConsoleService> console(
    do_GetService("@mozilla.org/consoleservice;1"));
  if (!console) {
    NS_WARNING("Failed to log message to console.");
    return;
  }
  nsAutoString msg(aMsg);
  console->LogStringMessage(msg.get());
}

void
DetailedPromise::MaybeReject(nsresult aArg, const nsACString& aReason)
{
  mResponded = true;

  LogToConsole(NS_ConvertUTF8toUTF16(aReason));

  nsRefPtr<DOMException> exception =
    DOMException::Create(aArg, aReason);
  Promise::MaybeRejectBrokenly(exception);
}

void
DetailedPromise::MaybeReject(ErrorResult&, const nsACString& aReason)
{
  NS_NOTREACHED("nsresult expected in MaybeReject()");
}

 already_AddRefed<DetailedPromise>
DetailedPromise::Create(nsIGlobalObject* aGlobal, ErrorResult& aRv)
{
  nsRefPtr<DetailedPromise> promise = new DetailedPromise(aGlobal);
  promise->CreateWrapper(aRv);
  return aRv.Failed() ? nullptr : promise.forget();
}

}
}
