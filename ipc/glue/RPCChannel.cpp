






































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
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(!ProcessingSyncMessage(),
                      "violation of sync handler invariant");
    NS_ABORT_IF_FALSE(msg->is_rpc(),
                      "can only Call() RPC messages here");

    MutexAutoLock lock(mMutex);

    if (!Connected())
        
        return false;

    mStack.push(*msg);
    msg->set_rpc_remote_stack_depth_guess(mRemoteStackDepthGuess);
    msg->set_rpc_local_stack_depth(StackDepth());

    
    
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
            if (!mPending.empty())
                RPC_DEBUGABORT("other side is malfunctioning");
            MutexAutoUnlock unlock(mMutex);

            SyncChannel::OnDispatchMessage(recvd);
            continue;
        }

        
        NS_ABORT_IF_FALSE(recvd.is_rpc(), "wtf???");

        
        if (recvd.is_reply()) {
            if (0 == mStack.size())
                RPC_DEBUGABORT("invalid RPC stack");

            const Message& outcall = mStack.top();

            if (recvd.type() != (outcall.type()+1) && !recvd.is_reply_error()) {
                
                RPC_DEBUGABORT("somebody's misbehavin'", "rpc", true);
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
                        if (seenBlocker)
                            RPC_DEBUGABORT("other side is malfunctioning",
                                           "sync", m.is_reply());
                        seenBlocker = true;

                        MessageLoop::current()->PostTask(
                            FROM_HERE,
                            NewRunnableMethod(this,
                                              &RPCChannel::OnDelegate, m));
                    }
                    else if (m.is_rpc()) {
                        if (seenBlocker)
                            RPC_DEBUGABORT("other side is malfunctioning",
                                           "rpc", m.is_reply());
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
                
                
                
                if (!mPending.empty())
                    RPC_DEBUGABORT("other side should have been blocked");
            }

            
            return !isError;
        }

        
        
        
        

        if (!(mPending.empty()
              || (1 == mPending.size()
                  && mPending.front().is_rpc()
                  && mPending.front().is_reply()
                  && 1 == StackDepth())))
            RPC_DEBUGABORT("other side is malfunctioning", "rpc");

        
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
    AssertWorkerThread();
    if (msg.is_sync())
        return SyncChannel::OnDispatchMessage(msg);
    else if (!msg.is_rpc())
        return AsyncChannel::OnDispatchMessage(msg);
    RPC_DEBUGABORT("fatal logic error");
}

void
RPCChannel::OnMaybeDequeueOne()
{
    AssertWorkerThread();
    Message recvd;  
    {
        MutexAutoLock lock(mMutex);

        if (mPending.empty())
            return;

        if (mPending.size() != 1)
            RPC_DEBUGABORT("should only have one msg");
        if (!(mPending.front().is_rpc() || mPending.front().is_sync()))
            RPC_DEBUGABORT("msg should be RPC or sync", "async");

        recvd = mPending.front();
        mPending.pop();
    }
    return recvd.is_sync() ?
        SyncChannel::OnDispatchMessage(recvd)
        : RPCChannel::OnIncall(recvd);
}

void
RPCChannel::OnIncall(const Message& call)
{
    AssertWorkerThread();
    
    
    
    ProcessIncall(call, 0);
}

void
RPCChannel::ProcessIncall(const Message& call, size_t stackDepth)
{
    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(call.is_rpc(),
                      "should have been handled by SyncChannel");

    
    
    
    if (call.rpc_remote_stack_depth_guess() != stackDepth) {
        NS_WARNING("RPC in-calls have raced!");

        
        
        
        
        
        if (!((1 == stackDepth && 0 == mRemoteStackDepthGuess)
              && (1 == call.rpc_local_stack_depth()
                  && 0 == call.rpc_remote_stack_depth_guess())))
            
            
            
            RPC_DEBUGABORT("fatal logic error");

        
        
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

        printf("  (%s won, so we're%sdeferring)\n",
               winner, defer ? " " : " not ");

        if (defer) {
            mWorkerLoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &RPCChannel::OnIncall, call));
            return;
        }

        
    }

    Message* reply = nsnull;

    ++mRemoteStackDepthGuess;
    Result rv =
        static_cast<RPCListener*>(mListener)->OnCallReceived(call, reply);
    --mRemoteStackDepthGuess;

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
    AssertIOThread();
    MutexAutoLock lock(mMutex);

    
    
    
    
    if (AwaitingSyncReply()
        && msg.is_sync()) {
        
        
        mRecvd = msg;
        mCvar.Notify();
        return;
    }

    
    

    if (0 == StackDepth()) {
        
        

        
        if (!msg.is_sync() && !msg.is_rpc()) {
            MutexAutoUnlock unlock(mMutex);
            return AsyncChannel::OnMessageReceived(msg);
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if (msg.is_sync()) {
            mPending.push(msg);

            mWorkerLoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &RPCChannel::OnMaybeDequeueOne));
            return;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        

        NS_ABORT_IF_FALSE(msg.is_rpc(), "should be RPC");

        mPending.push(msg);
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &RPCChannel::OnMaybeDequeueOne));
    }
    else {
        

        
        

        
        

        
        
        
        
        
        
        

        
        
        if (AwaitingSyncReply()
            && !msg.is_sync() && !msg.is_rpc()) {
            mPending.push(msg);
            return;
        }

        
        
        
        
        if (AwaitingSyncReply() ) {
            
            RPC_DEBUGABORT("the other side is malfunctioning",
                           "rpc", msg.is_reply());
            return;             
        }

        
        
        
        
        mPending.push(msg);
        mCvar.Notify();
    }
}


void
RPCChannel::OnChannelError()
{
    AssertIOThread();
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
