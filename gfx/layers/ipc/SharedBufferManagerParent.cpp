





#include "mozilla/layers/SharedBufferManagerParent.h"
#include "base/message_loop.h"          
#include "base/process.h"               
#include "base/process_util.h"          
#include "base/task.h"                  
#include "base/tracked.h"               
#include "base/thread.h"
#include "mozilla/ipc/MessageChannel.h" 
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/Transport.h"      
#include "nsIMemoryReporter.h"
#ifdef MOZ_WIDGET_GONK
#include "ui/PixelFormat.h"
#endif
#include "nsThreadUtils.h"

using namespace mozilla::ipc;
#ifdef MOZ_WIDGET_GONK
using namespace android;
#endif
using std::map;

namespace mozilla {
namespace layers {

class GrallocReporter MOZ_FINAL : public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS

  GrallocReporter()
  {
#ifdef DEBUG
    
    
    static bool hasRun = false;
    MOZ_ASSERT(!hasRun);
    hasRun = true;
#endif
  }

  NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                            nsISupports* aData)
  {
    return MOZ_COLLECT_REPORT(
      "gralloc", KIND_OTHER, UNITS_BYTES, sAmount,
"Special RAM that can be shared between processes and directly accessed by "
"both the CPU and GPU. Gralloc memory is usually a relatively precious "
"resource, with much less available than generic RAM. When it's exhausted, "
"graphics performance can suffer. This value can be incorrect because of race "
"conditions.");
  }

  static int64_t sAmount;
};

NS_IMPL_ISUPPORTS(GrallocReporter, nsIMemoryReporter)

int64_t GrallocReporter::sAmount = 0;

void InitGralloc() {
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  RegisterStrongMemoryReporter(new GrallocReporter());
}

map<base::ProcessId, SharedBufferManagerParent* > SharedBufferManagerParent::sManagers;
StaticAutoPtr<Monitor> SharedBufferManagerParent::sManagerMonitor;
int SharedBufferManagerParent::sBufferKey = 0;

SharedBufferManagerParent::SharedBufferManagerParent(Transport* aTransport, base::ProcessId aOwner, base::Thread* aThread)
  : mTransport(aTransport)
  , mThread(aThread)
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  , mBuffersMutex("BuffersMonitor")
#endif
{
  if (!sManagerMonitor)
    sManagerMonitor = new Monitor("Manager Monitor");

  MonitorAutoLock lock(*sManagerMonitor.get());
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread");
  if (!aThread->IsRunning())
    aThread->Start();
  mOwner = aOwner;
  sManagers[aOwner] = this;
}

SharedBufferManagerParent::~SharedBufferManagerParent()
{
  MonitorAutoLock lock(*sManagerMonitor.get());
  if (mTransport) {
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new DeleteTask<Transport>(mTransport));
  }
  sManagers.erase(mOwner);
  delete mThread;
}

void
SharedBufferManagerParent::ActorDestroy(ActorDestroyReason aWhy)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  mBuffers.clear();
#endif
}

static void
ConnectSharedBufferManagerInParentProcess(SharedBufferManagerParent* aManager,
                                          Transport* aTransport,
                                          base::ProcessHandle aOtherProcess)
{
  aManager->Open(aTransport, aOtherProcess, XRE_GetIOMessageLoop(), ipc::ParentSide);
}

PSharedBufferManagerParent* SharedBufferManagerParent::Create(Transport* aTransport, ProcessId aOtherProcess)
{
  ProcessHandle processHandle;
  if (!base::OpenProcessHandle(aOtherProcess, &processHandle)) {
    return nullptr;
  }

  base::Thread* thread = nullptr;
  if (sManagers.count(aOtherProcess) == 1) {
    thread = sManagers[aOtherProcess]->mThread;
  }
  else {
    char thrname[128];
    base::snprintf(thrname, 128, "BufMgrParent#%d", aOtherProcess);
    thread = new base::Thread(thrname);
  }
  SharedBufferManagerParent* manager = new SharedBufferManagerParent(aTransport, aOtherProcess, thread);
  if (!thread->IsRunning())
    thread->Start();
  thread->message_loop()->PostTask(FROM_HERE,
                                   NewRunnableFunction(ConnectSharedBufferManagerInParentProcess,
                                                       manager, aTransport, processHandle));
  return manager;
}

bool SharedBufferManagerParent::RecvAllocateGrallocBuffer(const IntSize& aSize, const uint32_t& aFormat, const uint32_t& aUsage, mozilla::layers::MaybeMagicGrallocBufferHandle* aHandle)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC

  *aHandle = null_t();

  if (aFormat == 0 || aUsage == 0) {
    printf_stderr("SharedBufferManagerParent::RecvAllocateGrallocBuffer -- format and usage must be non-zero");
    return true;
  }

  
  
  
  
  if (aSize.width > 4096 || aSize.height > 4096) {
    printf_stderr("SharedBufferManagerParent::RecvAllocateGrallocBuffer -- requested gralloc buffer is too big.");
    return false;
  }

  sp<GraphicBuffer> outgoingBuffer = new GraphicBuffer(aSize.width, aSize.height, aFormat, aUsage);
  if (!outgoingBuffer.get() || outgoingBuffer->initCheck() != NO_ERROR) {
    printf_stderr("SharedBufferManagerParent::RecvAllocateGrallocBuffer -- gralloc buffer allocation failed");
    return true;
  }

  GrallocBufferRef ref;
  ref.mOwner = mOwner;
  ref.mKey = ++sBufferKey;
  *aHandle = MagicGrallocBufferHandle(outgoingBuffer, ref);

  int bpp = 0;
  bpp = android::bytesPerPixel(outgoingBuffer->getPixelFormat());
  if (bpp > 0)
    GrallocReporter::sAmount += outgoingBuffer->getStride() * outgoingBuffer->getHeight() * bpp;
  else 
    GrallocReporter::sAmount += outgoingBuffer->getStride() * outgoingBuffer->getHeight() * 3 / 2;

  {
    MutexAutoLock lock(mBuffersMutex);
    mBuffers[sBufferKey] = outgoingBuffer;
  }
#endif
  return true;
}

bool SharedBufferManagerParent::RecvDropGrallocBuffer(const mozilla::layers::MaybeMagicGrallocBufferHandle& handle)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  NS_ASSERTION(handle.type() == MaybeMagicGrallocBufferHandle::TGrallocBufferRef, "We shouldn't interact with the real buffer!");
  int bufferKey = handle.get_GrallocBufferRef().mKey;
  sp<GraphicBuffer> buf = GetGraphicBuffer(bufferKey);
  MOZ_ASSERT(buf.get());
  MutexAutoLock lock(mBuffersMutex);
  NS_ASSERTION(mBuffers.count(bufferKey) == 1, "No such buffer");
  mBuffers.erase(bufferKey);

  if(!buf.get()) {
    printf_stderr("SharedBufferManagerParent::RecvDropGrallocBuffer -- invalid buffer key.");
    return true;
  }

  int bpp = 0;
  bpp = android::bytesPerPixel(buf->getPixelFormat());
  if (bpp > 0)
    GrallocReporter::sAmount -= buf->getStride() * buf->getHeight() * bpp;
  else 
    GrallocReporter::sAmount -= buf->getStride() * buf->getHeight() * 3 / 2;

#endif
  return true;
}

void SharedBufferManagerParent::DropGrallocBufferSync(SharedBufferManagerParent* mgr, mozilla::layers::SurfaceDescriptor aDesc)
{
  mgr->DropGrallocBufferImpl(aDesc);
}

void SharedBufferManagerParent::DropGrallocBuffer(mozilla::layers::SurfaceDescriptor aDesc)
{
  if (aDesc.type() != SurfaceDescriptor::TNewSurfaceDescriptorGralloc)
    return;

  if (PlatformThread::CurrentId() == mThread->thread_id()) {
    DropGrallocBufferImpl(aDesc);
  } else {
    mThread->message_loop()->PostTask(FROM_HERE,
                                      NewRunnableFunction(&DropGrallocBufferSync, this, aDesc));
  }
  return;
}

void SharedBufferManagerParent::DropGrallocBufferImpl(mozilla::layers::SurfaceDescriptor aDesc)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  MutexAutoLock lock(mBuffersMutex);
  int key = -1;
  MaybeMagicGrallocBufferHandle handle;
  if (aDesc.type() == SurfaceDescriptor::TNewSurfaceDescriptorGralloc)
    handle = aDesc.get_NewSurfaceDescriptorGralloc().buffer();
  else
    return;

  if (handle.type() == MaybeMagicGrallocBufferHandle::TGrallocBufferRef)
    key = handle.get_GrallocBufferRef().mKey;
  else if (handle.type() == MaybeMagicGrallocBufferHandle::TMagicGrallocBufferHandle)
    key = handle.get_MagicGrallocBufferHandle().mRef.mKey;

  NS_ASSERTION(key != -1, "Invalid buffer key");
  NS_ASSERTION(mBuffers.count(key) == 1, "No such buffer");
  mBuffers.erase(key);
  SendDropGrallocBuffer(handle);
#endif
}

MessageLoop* SharedBufferManagerParent::GetMessageLoop()
{
  return mThread->message_loop();
}

SharedBufferManagerParent* SharedBufferManagerParent::GetInstance(ProcessId id)
{
  MonitorAutoLock lock(*sManagerMonitor.get());
  NS_ASSERTION(sManagers.count(id) == 1, "No BufferManager for the process");
  return sManagers[id];
}

#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
android::sp<android::GraphicBuffer>
SharedBufferManagerParent::GetGraphicBuffer(int key)
{
  MutexAutoLock lock(mBuffersMutex);
  if (mBuffers.count(key) == 1) {
    return mBuffers[key];
  }
  else {
    
    return nullptr;
  }
}

android::sp<android::GraphicBuffer>
SharedBufferManagerParent::GetGraphicBuffer(GrallocBufferRef aRef)
{
  return GetInstance(aRef.mOwner)->GetGraphicBuffer(aRef.mKey);
}
#endif

IToplevelProtocol*
SharedBufferManagerParent::CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                                 base::ProcessHandle aPeerProcess,
                                 mozilla::ipc::ProtocolCloneContext* aCtx)
{
  for (unsigned int i = 0; i < aFds.Length(); i++) {
    if (aFds[i].protocolId() == unsigned(GetProtocolId())) {
      Transport* transport = OpenDescriptor(aFds[i].fd(),
                                            Transport::MODE_SERVER);
      PSharedBufferManagerParent* bufferManager = Create(transport, base::GetProcId(aPeerProcess));
      bufferManager->CloneManagees(this, aCtx);
      bufferManager->IToplevelProtocol::SetTransport(transport);
      return bufferManager;
    }
  }
  return nullptr;
}

} 
} 
