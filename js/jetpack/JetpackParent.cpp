




































#include "mozilla/jetpack/JetpackParent.h"
#include "mozilla/jetpack/Handle.h"

#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIVariant.h"
#include "nsIXPConnect.h"

namespace mozilla {
namespace jetpack {

JetpackParent::JetpackParent(JSContext* cx)
  : mSubprocess(new JetpackProcessParent())
  , mContext(cx)
{
  mSubprocess->Launch();
  Open(mSubprocess->GetChannel(),
       mSubprocess->GetChildProcessHandle());
}

JetpackParent::~JetpackParent()
{
  if (mSubprocess)
    Destroy();
}

NS_IMPL_ISUPPORTS1(JetpackParent, nsIJetpack)

static nsresult
ReadFromURI(const nsAString& aURI,
            nsCString* content)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri),
                          NS_ConvertUTF16toUTF8(aURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  NS_NewChannel(getter_AddRefs(channel), uri);
  NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);

  nsCOMPtr<nsIInputStream> input;
  rv = channel->Open(getter_AddRefs(input));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(input, "Channel opened successfully but stream was null?");

  char buffer[256];
  PRUint32 avail = 0;
  input->Available(&avail);
  if (avail) {
    PRUint32 read = 0;
    while (NS_SUCCEEDED(input->Read(buffer, sizeof(buffer), &read)) && read) {
      content->Append(buffer, read);
      read = 0;
    }
  }

  return NS_OK;
}

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

  for (PRUint32 i = 1; i < argc; ++i)
    if (!jsval_to_Variant(cx, argv[i], data.AppendElement()))
      return NS_ERROR_INVALID_ARG;

  if (!SendSendMessage(nsString(aMessageName), data))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
JetpackParent::RegisterReceiver(const nsAString& aMessageName,
                                jsval aReceiver)
{
  return JetpackActorCommon::RegisterReceiver(mContext,
                                              nsString(aMessageName),
                                              aReceiver);
}

NS_IMETHODIMP
JetpackParent::UnregisterReceiver(const nsAString& aMessageName,
                                  jsval aReceiver)
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
JetpackParent::LoadImplementation(const nsAString& aURI)
{
  nsCString code;
  nsresult rv = ReadFromURI(aURI, &code);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!code.IsEmpty() &&
      !SendLoadImplementation(code))
    rv = NS_ERROR_FAILURE;

  return rv;
}

NS_IMETHODIMP
JetpackParent::LoadUserScript(const nsAString& aURI)
{
  nsCString code;
  nsresult rv = ReadFromURI(aURI, &code);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!code.IsEmpty() &&
      !SendLoadUserScript(code))
    rv = NS_ERROR_FAILURE;

  return rv;
}

bool
JetpackParent::RecvSendMessage(const nsString& messageName,
                               const nsTArray<Variant>& data)
{
  JSAutoRequest request(mContext);
  return JetpackActorCommon::RecvMessage(mContext, messageName, data, NULL);
}

bool
JetpackParent::RecvCallMessage(const nsString& messageName,
                               const nsTArray<Variant>& data,
                               nsTArray<Variant>* results)
{
  JSAutoRequest request(mContext);
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

  JSObject* hobj = handle->ToJSObject(mContext);
  if (!hobj)
    return NS_ERROR_FAILURE;

  return xpc->JSToVariant(mContext, OBJECT_TO_JSVAL(hobj), aResult);
}

NS_IMETHODIMP
JetpackParent::Destroy()
{
  if (!mSubprocess)
    return NS_ERROR_NOT_INITIALIZED;

  Close();
  XRE_GetIOMessageLoop()
    ->PostTask(FROM_HERE, new DeleteTask<JetpackProcessParent>(mSubprocess));
  mSubprocess = NULL;

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

} 
} 
