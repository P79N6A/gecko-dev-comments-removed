






































#include "mozilla/ipc/RPCChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"

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

bool
RPCChannel::Call(Message* msg, Message* reply)
{
    NS_ABORT_IF_FALSE(!ProcessingSyncMessage(),
                      "violation of sync handler invariant");
    NS_ASSERTION(ChannelConnected == mChannelState,
                 "trying to Send() to a channel not yet open");
    NS_PRECONDITION(msg->is_rpc(),
                    "can only Call() RPC messages here");

    mMutex.Lock();

    msg->set_rpc_remote_stack_depth(mRemoteStackDepth);
    mPending.push(*msg);

    
    
    AsyncChannel::Send(msg);

    while (1) {
        
        
        
        
        
        mCvar.Wait();

        Message recvd = mPending.top();
        mPending.pop();

        
        if (!recvd.is_sync() && !recvd.is_rpc()) {
            MutexAutoUnlock unlock(mMutex);

            AsyncChannel::OnDispatchMessage(recvd);
            continue;
        }

        
        
        
        if (recvd.is_sync()) {
            MutexAutoUnlock unlock(mMutex);

            SyncChannel::OnDispatchMessage(recvd);
            continue;
        }

        
        NS_ABORT_IF_FALSE(recvd.is_rpc(), "wtf???");

        
        if (recvd.is_reply()) {
            NS_ABORT_IF_FALSE(0 < mPending.size(), "invalid RPC stack");

            const Message& pending = mPending.top();

            if (recvd.type() != (pending.type()+1) && !recvd.is_reply_error()) {
                
                NS_ABORT_IF_FALSE(0, "somebody's misbehavin'");
            }

            
            
            mPending.pop();

            bool isError = recvd.is_reply_error();
            if (!isError) {
                *reply = recvd;
            }

            mMutex.Unlock();
            return !isError;
        }
        
        else {
            
            size_t stackDepth = StackDepth();

            MutexAutoUnlock unlock(mMutex);
            
            ProcessIncall(recvd, stackDepth);
            
        }
    }

    return true;
}

void
RPCChannel::OnIncall(const Message& call)
{
    
    
    
    ProcessIncall(call, 0);
}

void
RPCChannel::ProcessIncall(const Message& call, size_t stackDepth)
{
    mMutex.AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(call.is_rpc(),
                      "should have been handled by SyncChannel");

    
    
    NS_ASSERTION(stackDepth == call.rpc_remote_stack_depth(),
                 "RPC in-calls have raced!");

    Message* reply = nsnull;

    ++mRemoteStackDepth;
    Result rv =
        static_cast<RPCListener*>(mListener)->OnCallReceived(call, reply);
    --mRemoteStackDepth;

    switch (rv) {
    case MsgProcessed:
        mIOLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &RPCChannel::OnSendReply,
                                            reply));
        return;

    case MsgNotKnown:
    case MsgNotAllowed:
    case MsgPayloadError:
    case MsgRouteError:
    case MsgValueError:
        delete reply;
        reply = new Message();
        reply->set_rpc();
        reply->set_reply();
        reply->set_reply_error();
        mIOLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &RPCChannel::OnSendReply,
                                            reply));
        
        return;

    default:
        NOTREACHED();
        return;
    }
}






void
RPCChannel::OnMessageReceived(const Message& msg)
{
    MutexAutoLock lock(mMutex);

    if (0 == StackDepth()) {
        
        
        
        
        
        
        
        if (!msg.is_rpc()) {
            
            return SyncChannel::OnMessageReceived(msg);
        }

        

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        mWorkerLoop->PostTask(FROM_HERE,
                              NewRunnableMethod(this,
                                                &RPCChannel::OnIncall, msg));
    }
    else {
        

        
        

        
        
        
        
        if (AwaitingSyncReply()
            && msg.is_sync()) {
            
            
            mRecvd = msg;
            mCvar.Notify();
            return;
        }

        
        
        if (AwaitingSyncReply()
            && !msg.is_sync() && !msg.is_rpc()) {
            
            return AsyncChannel::OnMessageReceived(msg);
        }

        
        
        
        
        if (AwaitingSyncReply() ) {
            
            NS_RUNTIMEABORT("the other side is malfunctioning");
            return;             
        }

        
        
        
        
        mPending.push(msg);
        mCvar.Notify();
    }
}


} 
} 
