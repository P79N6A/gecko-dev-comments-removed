




































#include "mozilla/jetpack/JetpackParent.h"
#include "mozilla/jetpack/Handle.h"
#include "base/process_util.h"

#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIVariant.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

namespace mozilla {
namespace jetpack {

JetpackParent::JetpackParent(JSContext* cx)
  : mSubprocess(new JetpackProcessParent())
  , mContext(cx)
  , mTaskFactory(this)
{
  mSubprocess->Launch();
  Open(mSubprocess->GetChannel(),
       mSubprocess->GetChildProcessHandle());
}

JetpackParent::~JetpackParent()
{
  if (mSubprocess)
    Destroy();

  if (OtherProcess())
    base::CloseProcessHandle(OtherProcess());
}

NS_IMPL_ISUPPORTS1(JetpackParent, nsIJetpack)

NS_IMETHODIMP
JetpackParent::SendMessage(const nsAString& aMessageName)
{
  nsresult rv;
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAXPCNativeCallContext* ncc = NULL;
  rv = xpc->GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  JSContext* cx;
  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 argc;
  rv = ncc->GetArgc(&argc);
  NS_ENSURE_SUCCESS(rv, rv);

  jsval* argv;
  rv = ncc->GetArgvPtr(&argv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<Variant> data;
  NS_ENSURE_TRUE(data.SetCapacity(argc), NS_ERROR_OUT_OF_MEMORY);

  JSAutoRequest request(cx);

  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, JS_GetGlobalObject(cx)))
    return false;

  for (PRUint32 i = 1; i < argc; ++i)
    if (!jsval_to_Variant(cx, argv[i], data.AppendElement()))
      return NS_ERROR_INVALID_ARG;

  InfallibleTArray<Variant> dataForSend;
  dataForSend.SwapElements(data);
  if (!SendSendMessage(nsString(aMessageName), dataForSend))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
JetpackParent::RegisterReceiver(const nsAString& aMessageName,
                                const jsval &aReceiver)
{
  return JetpackActorCommon::RegisterReceiver(mContext,
                                              nsString(aMessageName),
                                              aReceiver);
}

NS_IMETHODIMP
JetpackParent::UnregisterReceiver(const nsAString& aMessageName,
                                  const jsval &aReceiver)
{
  JetpackActorCommon::UnregisterReceiver(nsString(aMessageName),
                                         aReceiver);
  return NS_OK;
}

NS_IMETHODIMP
JetpackParent::UnregisterReceivers(const nsAString& aMessageName)
{
  JetpackActorCommon::UnregisterReceivers(nsString(aMessageName));
  return NS_OK;
}

NS_IMETHODIMP
JetpackParent::EvalScript(const nsAString& aScript)
{
  if (!SendEvalScript(nsString(aScript)))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

class AutoCXPusher
{
public:
  AutoCXPusher(JSContext* cx)
    : mCXStack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"))
  {
    NS_ASSERTION(mCXStack, "No JS context stack?");
    if (mCXStack)
      mCXStack->Push(cx);
  }
  ~AutoCXPusher()
  {
    if (mCXStack)
      mCXStack->Pop(NULL);
  }

private:
  nsCOMPtr<nsIJSContextStack> mCXStack;
  JSContext* mCX;
};

void
JetpackParent::ActorDestroy(ActorDestroyReason why)
{
  switch (why) {
    case AbnormalShutdown: {
      nsAutoString dumpID;

#ifdef MOZ_CRASHREPORTER
      nsCOMPtr<nsILocalFile> crashDump;
      TakeMinidump(getter_AddRefs(crashDump)) &&
        CrashReporter::GetIDFromMinidump(crashDump, dumpID);
#endif

      MessageLoop::current()->
        PostTask(FROM_HERE,
                 mTaskFactory.NewRunnableMethod(
                   &JetpackParent::DispatchFailureMessage,
                   dumpID));
      break;
    }

    case NormalShutdown:
      break;

    default:
      NS_ERROR("Unexpected actordestroy reason for toplevel actor.");
  }  

  XRE_GetIOMessageLoop()
    ->PostTask(FROM_HERE, new DeleteTask<JetpackProcessParent>(mSubprocess));
  mSubprocess = NULL;
}

bool
JetpackParent::RecvSendMessage(const nsString& messageName,
                               const InfallibleTArray<Variant>& data)
{
  AutoCXPusher cxp(mContext);
  JSAutoRequest request(mContext);

  JSAutoEnterCompartment ac;
  if (!ac.enter(mContext, JS_GetGlobalObject(mContext)))
    return false;

  return JetpackActorCommon::RecvMessage(mContext, messageName, data, NULL);
}

bool
JetpackParent::AnswerCallMessage(const nsString& messageName,
                                 const InfallibleTArray<Variant>& data,
                                 InfallibleTArray<Variant>* results)
{
  AutoCXPusher cxp(mContext);
  JSAutoRequest request(mContext);

  JSAutoEnterCompartment ac;
  if (!ac.enter(mContext, JS_GetGlobalObject(mContext)))
    return false;

  return JetpackActorCommon::RecvMessage(mContext, messageName, data, results);
}

NS_IMETHODIMP
JetpackParent::CreateHandle(nsIVariant** aResult)
{
  HandleParent* handle =
    static_cast<HandleParent*>(SendPHandleConstructor());
  NS_ENSURE_TRUE(handle, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest request(mContext);

  JSAutoEnterCompartment ac;
  if (!ac.enter(mContext, JS_GetGlobalObject(mContext)))
    return false;

  JSObject* hobj = handle->ToJSObject(mContext);
  if (!hobj)
    return NS_ERROR_FAILURE;

  return xpc->JSToVariant(mContext, OBJECT_TO_JSVAL(hobj), aResult);
}

NS_IMETHODIMP
JetpackParent::Destroy()
{
  if (mSubprocess)
    Close();

  NS_ASSERTION(!mSubprocess, "ActorDestroy should have been called.");
  return NS_OK;
}

PHandleParent*
JetpackParent::AllocPHandle()
{
  return new HandleParent();
}

bool
JetpackParent::DeallocPHandle(PHandleParent* actor)
{
  delete actor;
  return true;
}

void
JetpackParent::OnChannelConnected(int32 pid) 
{
  ProcessHandle handle;
  if (!base::OpenPrivilegedProcessHandle(pid, &handle))
    NS_RUNTIMEABORT("can't open handle to child process");

  SetOtherProcess(handle);
}

void
JetpackParent::DispatchFailureMessage(const nsString& aDumpID)
{
  KeyValue kv(NS_LITERAL_STRING("dumpID"), PrimVariant(aDumpID));
  InfallibleTArray<KeyValue> keyvalues;
  keyvalues.AppendElement(kv);

  CompVariant object(keyvalues);

  InfallibleTArray<Variant> arguments;
  arguments.AppendElement(object);

  RecvSendMessage(NS_LITERAL_STRING("core:process-error"), arguments);
}

} 
} 
