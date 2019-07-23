






































#include "mozilla/ipc/RPCChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"

#define RPC_ASSERT(_cond, ...)                                      \
    do {                                                            \
        if (!(_cond))                                               \
            DebugAbort(__FILE__, __LINE__, #_cond,## __VA_ARGS__);  \
    } while (0)

using mozilla::MutexAutoLock;
using mozilla::MutexAutoUnlock;

template<>
struct RunnableMethodTraits<mozilla::ipc::RPCChannel>
{
    static void RetainCallee(mozilla::ipc::RPCChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::RPCChannel* obj) { }
};

namespace mozilla {
namespace ipc {

RPCChannel::RPCChannel(RPCListener* aListener,
                       RacyRPCPolicy aPolicy)
  : SyncChannel(aListener),
    mPending(),
    mStack(),
    mDeferred(),
    mRemoteStackDepthGuess(0),
    mRacePolicy(aPolicy)
{
    MOZ_COUNT_CTOR(RPCChannel);
}

RPCChannel::~RPCChannel()
{
    MOZ_COUNT_DTOR(RPCChannel);
    
}

bool
RPCChannel::Call(Message* msg, Message* reply)
{
    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    RPC_ASSERT(!ProcessingSyncMessage(),
               "violation of sync handler invariant");
    RPC_ASSERT(msg->is_rpc(), "can only Call() RPC messages here");

    MutexAutoLock lock(mMutex);

    if (!Connected()) {
        ReportConnectionError("RPCChannel");
        return false;
    }

    mStack.push(*msg);
    msg->set_rpc_remote_stack_depth_guess(mRemoteStackDepthGuess);
    msg->set_rpc_local_stack_depth(StackDepth());

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RPCChannel::OnSend, msg));

    while (1) {
        
        
        MaybeProcessDeferredIncall();

        
        
        while (Connected() && mPending.empty()) {
            WaitForNotify();
        }

        if (!Connected()) {
            ReportConnectionError("RPCChannel");
            return false;
        }

        Message recvd = mPending.front();
        mPending.pop();

        if (!recvd.is_sync() && !recvd.is_rpc()) {
            MutexAutoUnlock unlock(mMutex);
            AsyncChannel::OnDispatchMessage(recvd);
            continue;
        }

        if (recvd.is_sync()) {
            RPC_ASSERT(mPending.empty(),
                       "other side should have been blocked");
            MutexAutoUnlock unlock(mMutex);
            SyncChannel::OnDispatchMessage(recvd);
            continue;
        }

        NS_ABORT_IF_FALSE(recvd.is_rpc(), "wtf???");

        if (recvd.is_reply()) {
            RPC_ASSERT(0 < mStack.size(), "invalid RPC stack");

            const Message& outcall = mStack.top();

            
            RPC_ASSERT(
                recvd.type() == (outcall.type()+1) || recvd.is_reply_error(),
                "somebody's misbehavin'", "rpc", true);

            
            
            mStack.pop();

            bool isError = recvd.is_reply_error();
            if (!isError) {
                *reply = recvd;
            }

            if (0 == StackDepth())
                
                
                
                
                
                EnqueuePendingMessages();

            
            return !isError;
        }

        

        
        size_t stackDepth = StackDepth();
        {
            MutexAutoUnlock unlock(mMutex);
            
            Incall(recvd, stackDepth);
            
        }
    }

    return true;
}

void
RPCChannel::MaybeProcessDeferredIncall()
{
    AssertWorkerThread();
    mMutex.AssertCurrentThreadOwns();

    if (mDeferred.empty())
        return;

    size_t stackDepth = StackDepth();

    
    RPC_ASSERT(mDeferred.top().rpc_remote_stack_depth_guess() <= stackDepth,
               "fatal logic error");

    if (mDeferred.top().rpc_remote_stack_depth_guess() < stackDepth)
        return;

    
    Message call = mDeferred.top();
    mDeferred.pop();

    
    RPC_ASSERT(0 < mRemoteStackDepthGuess, "fatal logic error");
    --mRemoteStackDepthGuess;

    MutexAutoUnlock unlock(mMutex);
    fprintf(stderr, "  (processing deferred in-call)\n");
    Incall(call, stackDepth);
}

void
RPCChannel::EnqueuePendingMessages()
{
    AssertWorkerThread();
    mMutex.AssertCurrentThreadOwns();
    RPC_ASSERT(mDeferred.empty() || 1 == mDeferred.size(),
               "expected mDeferred to have 0 or 1 items");

    if (!mDeferred.empty())
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &RPCChannel::OnMaybeDequeueOne));

    
    

    for (size_t i = 0; i < mPending.size(); ++i)
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &RPCChannel::OnMaybeDequeueOne));
}

void
RPCChannel::OnMaybeDequeueOne()
{
    
    

    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    RPC_ASSERT(mDeferred.empty() || 1 == mDeferred.size(),
               "expected mDeferred to have 0 or 1 items, but it has %lu");

    Message recvd;
    {
        MutexAutoLock lock(mMutex);

        if (!mDeferred.empty())
            return MaybeProcessDeferredIncall();

        if (mPending.empty())
            return;

        recvd = mPending.front();
        mPending.pop();
    }

    if (recvd.is_rpc())
        return Incall(recvd, 0);
    else if (recvd.is_sync())
        return SyncChannel::OnDispatchMessage(recvd);
    else
        return AsyncChannel::OnDispatchMessage(recvd);
}

void
RPCChannel::Incall(const Message& call, size_t stackDepth)
{
    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    RPC_ASSERT(call.is_rpc() && !call.is_reply(), "wrong message type");

    
    
    
    if (call.rpc_remote_stack_depth_guess() != stackDepth) {
        NS_WARNING("RPC in-calls have raced!");

        RPC_ASSERT(call.rpc_remote_stack_depth_guess() < stackDepth,
                   "fatal logic error");
        RPC_ASSERT(1 == (stackDepth - call.rpc_remote_stack_depth_guess()),
                   "got more than 1 RPC message out of sync???");
        RPC_ASSERT(1 == (call.rpc_local_stack_depth() -mRemoteStackDepthGuess),
                   "RPC unexpected not symmetric");

        
        
        bool defer;
        const char* winner;
        switch (mRacePolicy) {
        case RRPChildWins:
            winner = "child";
            defer = mChild;
            break;
        case RRPParentWins:
            winner = "parent";
            defer = !mChild;
            break;
        case RRPError:
            NS_RUNTIMEABORT("NYI: 'Error' RPC race policy");
            return;
        default:
            NS_RUNTIMEABORT("not reached");
            return;
        }

        fprintf(stderr, "  (%s won, so we're%sdeferring)\n",
                winner, defer ? " " : " not ");

        if (defer) {
            
            
            ++mRemoteStackDepthGuess; 
            mDeferred.push(call);
            return;
        }

        
        
        
        
    }

    DispatchIncall(call);
}

void
RPCChannel::DispatchIncall(const Message& call)
{
    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    RPC_ASSERT(call.is_rpc() && !call.is_reply(),
               "wrong message type");

    Message* reply = nsnull;

    ++mRemoteStackDepthGuess;
    Result rv =
        static_cast<RPCListener*>(mListener)->OnCallReceived(call, reply);
    --mRemoteStackDepthGuess;

    if (!MaybeHandleError(rv, "RPCChannel")) {
        delete reply;
        reply = new Message();
        reply->set_rpc();
        reply->set_reply();
        reply->set_reply_error();
    }

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RPCChannel::OnSend, reply));
}


void
RPCChannel::DebugAbort(const char* file, int line, const char* cond,
                       const char* why,
                       const char* type, bool reply)
{
    fprintf(stderr,
            "###!!! [RPCChannel][%s][%s:%d] "
            "Assertion (%s) failed.  %s (triggered by %s%s)\n",
            mChild ? "Child" : "Parent",
            file, line, cond,
            why,
            type, reply ? "reply" : "");
    
    fprintf(stderr, "  local RPC stack size: %lu\n",
            mStack.size());
    fprintf(stderr, "  remote RPC stack guess: %lu\n",
            mRemoteStackDepthGuess);
    fprintf(stderr, "  deferred stack size: %lu\n",
            mDeferred.size());
    fprintf(stderr, "  Pending queue size: %lu, front to back:\n",
            mPending.size());
    while (!mPending.empty()) {
        fprintf(stderr, "    [ %s%s ]\n",
                mPending.front().is_rpc() ? "rpc" :
                (mPending.front().is_sync() ? "sync" : "async"),
                mPending.front().is_reply() ? "reply" : "");
        mPending.pop();
    }

    NS_RUNTIMEABORT(why);
}






void
RPCChannel::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    MutexAutoLock lock(mMutex);

    
    
    if (AwaitingSyncReply() && msg.is_sync()) {
        
        mRecvd = msg;
        NotifyWorkerThread();
        return;
    }

    mPending.push(msg);

    if (0 == StackDepth())
        
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &RPCChannel::OnMaybeDequeueOne));
    else
        NotifyWorkerThread();
}


void
RPCChannel::OnChannelError()
{
    AssertIOThread();

    AsyncChannel::OnChannelError();

    
    MutexAutoLock lock(mMutex);
    if (AwaitingSyncReply()
        || 0 < StackDepth())
        NotifyWorkerThread();
}


} 
} 

