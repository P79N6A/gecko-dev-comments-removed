






































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
    NS_ABORT_IF_FALSE(msg->is_rpc(),
                      "can only Call() RPC messages here");

    if (!Connected())
        
        return false;

    MutexAutoLock lock(mMutex);

    msg->set_rpc_remote_stack_depth(mRemoteStackDepth);
    mStack.push(*msg);

    
    
    AsyncChannel::Send(msg);

    while (1) {
        
        
        while (Connected() && mPending.empty()) {
            mCvar.Wait();
        }

        if (!Connected())
            
            return false;

        Message recvd = mPending.front();
        mPending.pop();

        
        if (!recvd.is_sync() && !recvd.is_rpc()) {
            MutexAutoUnlock unlock(mMutex);

            AsyncChannel::OnDispatchMessage(recvd);
            continue;
        }

        
        
        
        if (recvd.is_sync()) {
            NS_ABORT_IF_FALSE(mPending.empty(),
                              "other side is malfunctioning");
            MutexAutoUnlock unlock(mMutex);

            SyncChannel::OnDispatchMessage(recvd);
            continue;
        }

        
        NS_ABORT_IF_FALSE(recvd.is_rpc(), "wtf???");

        
        if (recvd.is_reply()) {
            NS_ABORT_IF_FALSE(0 < mStack.size(), "invalid RPC stack");

            const Message& outcall = mStack.top();

            if (recvd.type() != (outcall.type()+1) && !recvd.is_reply_error()) {
                
                NS_ABORT_IF_FALSE(0, "somebody's misbehavin'");
            }

            
            
            mStack.pop();

            bool isError = recvd.is_reply_error();
            if (!isError) {
                *reply = recvd;
            }

            if (0 == StackDepth()) {
                
                
                
                
                bool seenBlocker = false;

                
                while (!mPending.empty()) {
                    Message m = mPending.front();
                    mPending.pop();

                    if (m.is_sync()) {
                        NS_ABORT_IF_FALSE(!seenBlocker,
                                          "other side is malfunctioning");
                        seenBlocker = true;

                        MessageLoop::current()->PostTask(
                            FROM_HERE,
                            NewRunnableMethod(this,
                                              &RPCChannel::OnDelegate, m));
                    }
                    else if (m.is_rpc()) {
                        NS_ABORT_IF_FALSE(!seenBlocker,
                                          "other side is malfunctioning");
                        seenBlocker = true;

                        MessageLoop::current()->PostTask(
                            FROM_HERE,
                            NewRunnableMethod(this,
                                              &RPCChannel::OnIncall,
                                              m));
                    }
                    else {
                        MessageLoop::current()->PostTask(
                            FROM_HERE,
                            NewRunnableMethod(this,
                                              &RPCChannel::OnDelegate, m));
                    }
                }
            }
            else {
                
                
                
                if (mPending.size() > 0) {
                    NS_RUNTIMEABORT("other side should have been blocked");
                }
            }

            
            return !isError;
        }

        
        NS_ABORT_IF_FALSE(mPending.empty(),
                          "other side is malfunctioning");

        
        size_t stackDepth = StackDepth();
        {
            MutexAutoUnlock unlock(mMutex);
            
            ProcessIncall(recvd, stackDepth);
            
        }
    }

    return true;
}

void
RPCChannel::OnDelegate(const Message& msg)
{
    if (msg.is_sync())
        return SyncChannel::OnDispatchMessage(msg);
    else if (!msg.is_rpc())
        return AsyncChannel::OnDispatchMessage(msg);
    NS_RUNTIMEABORT("fatal logic error");
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
        break;

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
        
        break;

    default:
        NOTREACHED();
        return;
    }

    mIOLoop->PostTask(FROM_HERE,
                      NewRunnableMethod(this,
                                        &RPCChannel::OnSendReply,
                                        reply));

}






void
RPCChannel::OnMessageReceived(const Message& msg)
{
    MutexAutoLock lock(mMutex);

    if (0 == StackDepth()) {
        
        
        
        
        
        
        
        if (!msg.is_rpc()) {
            MutexAutoUnlock unlock(mMutex);
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
            mPending.push(msg);
            return;
        }

        
        
        
        
        if (AwaitingSyncReply() ) {
            
            NS_RUNTIMEABORT("the other side is malfunctioning");
            return;             
        }

        
        
        
        
        mPending.push(msg);
        mCvar.Notify();
    }
}


void
RPCChannel::OnChannelError()
{
    {
        MutexAutoLock lock(mMutex);

        mChannelState = ChannelError;

        if (AwaitingSyncReply()
            || 0 < StackDepth()) {
            mCvar.Notify();
        }
    }

    

    return AsyncChannel::OnChannelError();
}


} 
} 
