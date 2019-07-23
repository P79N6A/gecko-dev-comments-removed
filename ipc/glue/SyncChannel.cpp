






































#include "mozilla/ipc/SyncChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"

using mozilla::MutexAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::SyncChannel>
{
    static void RetainCallee(mozilla::ipc::SyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::SyncChannel* obj) { }
};

namespace mozilla {
namespace ipc {

SyncChannel::SyncChannel(SyncListener* aListener)
  : AsyncChannel(aListener),
    mPendingReply(0),
    mProcessingSyncMessage(false)
{
  MOZ_COUNT_CTOR(SyncChannel);
}

SyncChannel::~SyncChannel()
{
    MOZ_COUNT_DTOR(SyncChannel);
    
}


bool SyncChannel::sIsPumpingMessages = false;

bool
SyncChannel::Send(Message* msg, Message* reply)
{
    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(!ProcessingSyncMessage(),
                      "violation of sync handler invariant");
    NS_ABORT_IF_FALSE(msg->is_sync(), "can only Send() sync messages here");

    MutexAutoLock lock(mMutex);

    if (!Connected()) {
        ReportConnectionError("SyncChannel");
        return false;
    }

    mPendingReply = msg->type() + 1;
    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &SyncChannel::OnSend, msg));

    
    WaitForNotify();

    if (!Connected()) {
        ReportConnectionError("SyncChannel");
        return false;
    }

    
    
    
    
    

    
    NS_ABORT_IF_FALSE(mRecvd.is_sync() && mRecvd.is_reply() &&
                      (mPendingReply == mRecvd.type() || mRecvd.is_reply_error()),
                      "unexpected sync message");

    mPendingReply = 0;
    *reply = mRecvd;

    return true;
}

void
SyncChannel::OnDispatchMessage(const Message& msg)
{
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(msg.is_sync(), "only sync messages here");
    NS_ABORT_IF_FALSE(!msg.is_reply(), "wasn't awaiting reply");

    Message* reply = 0;

    mProcessingSyncMessage = true;
    Result rv =
        static_cast<SyncListener*>(mListener)->OnMessageReceived(msg, reply);
    mProcessingSyncMessage = false;

    if (!MaybeHandleError(rv, "SyncChannel")) {
        
        delete reply;
        reply = new Message();
        reply->set_sync();
        reply->set_reply();
        reply->set_reply_error();
    }

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &SyncChannel::OnSend, reply));
}






void
SyncChannel::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    if (!msg.is_sync()) {
        return AsyncChannel::OnMessageReceived(msg);
    }

    MutexAutoLock lock(mMutex);

    if (!AwaitingSyncReply()) {
        
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &SyncChannel::OnDispatchMessage, msg));
    }
    else {
        
        mRecvd = msg;
        NotifyWorkerThread();
    }
}

void
SyncChannel::OnChannelError()
{
    AssertIOThread();

    AsyncChannel::OnChannelError();

    MutexAutoLock lock(mMutex);
    if (AwaitingSyncReply())
        NotifyWorkerThread();
}








#ifndef OS_WIN

void
SyncChannel::WaitForNotify()
{
    mCvar.Wait();
}

void
SyncChannel::NotifyWorkerThread()
{
    mCvar.Notify();
}

#endif  


} 
} 
