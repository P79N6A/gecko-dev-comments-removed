











#include "xpcprivate.h"
#include "MainThreadUtils.h"
#include "mozilla/Assertions.h"
#include "nsGlobalWindow.h"
#include "nsCycleCollectionParticipant.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsScriptErrorWithStack)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsScriptErrorWithStack)
  tmp->mStack = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsScriptErrorWithStack)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsScriptErrorWithStack)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mStack)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsScriptErrorWithStack)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsScriptErrorWithStack)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsScriptErrorWithStack)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIConsoleMessage)
  NS_INTERFACE_MAP_ENTRY(nsIScriptError)
NS_INTERFACE_MAP_END

nsScriptErrorWithStack::nsScriptErrorWithStack(JS::HandleObject aStack)
    :  nsScriptError(),
       mStack(aStack)
{
    MOZ_ASSERT(NS_IsMainThread(), "You can't use this class on workers.");
    mozilla::HoldJSObjects(this);
}

nsScriptErrorWithStack::~nsScriptErrorWithStack() {
    mozilla::DropJSObjects(this);
}

NS_IMETHODIMP
nsScriptErrorWithStack::Init(const nsAString& message,
                             const nsAString& sourceName,
                             const nsAString& sourceLine,
                             uint32_t lineNumber,
                             uint32_t columnNumber,
                             uint32_t flags,
                             const char* category)
{
  MOZ_CRASH("nsScriptErrorWithStack requires to be initialized with a document, by using InitWithWindowID");
}

NS_IMETHODIMP
nsScriptErrorWithStack::GetStack(JS::MutableHandleValue aStack) {
    aStack.setObjectOrNull(mStack);
    return NS_OK;
}
