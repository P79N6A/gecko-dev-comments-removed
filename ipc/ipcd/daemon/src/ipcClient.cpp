




































#include "ipcLog.h"
#include "ipcClient.h"
#include "ipcMessage.h"
#include "ipcModuleReg.h"
#include "ipcd.h"
#include "ipcm.h"

#if defined(XP_UNIX) || defined(XP_OS2)
#include "prio.h"
#endif

PRUint32 ipcClient::gLastID = 0;







void
ipcClient::Init()
{
    mID = ++gLastID;

    
    mTargets.Append(IPCM_TARGET);

    
    
    
}




void
ipcClient::Finalize()
{
    IPC_NotifyClientDown(this);

    mNames.DeleteAll();
    mTargets.DeleteAll();

#if defined(XP_UNIX) || defined(XP_OS2)
    mInMsg.Reset();
    mOutMsgQ.DeleteAll();
#endif
}

void
ipcClient::AddName(const char *name)
{
    LOG(("adding client name: %s\n", name));

    if (HasName(name))
        return;

    mNames.Append(name);
}

void
ipcClient::DelName(const char *name)
{
    LOG(("deleting client name: %s\n", name));

    mNames.FindAndDelete(name);
}

void
ipcClient::AddTarget(const nsID &target)
{
    LOG(("adding client target\n"));

    if (HasTarget(target))
        return;

    mTargets.Append(target);
}

void
ipcClient::DelTarget(const nsID &target)
{
    LOG(("deleting client target\n"));

    
    
    
    if (!target.Equals(IPCM_TARGET))
        mTargets.FindAndDelete(target);
}

#if defined(XP_UNIX) || defined(XP_OS2)













int
ipcClient::Process(PRFileDesc *fd, int inFlags)
{
    if (inFlags & (PR_POLL_ERR    | PR_POLL_HUP |
                   PR_POLL_EXCEPT | PR_POLL_NVAL)) {
        LOG(("client socket appears to have closed\n"));
        return 0;
    }

    
    int outFlags = PR_POLL_READ;

    if (inFlags & PR_POLL_READ) {
        LOG(("client socket is now readable\n"));

        char buf[1024]; 
        PRInt32 n;

        
        

        n = PR_Read(fd, buf, sizeof(buf));
        if (n <= 0)
            return 0; 

        const char *ptr = buf;
        while (n) {
            PRUint32 nread;
            PRBool complete;

            if (mInMsg.ReadFrom(ptr, PRUint32(n), &nread, &complete) == PR_FAILURE) {
                LOG(("message appears to be malformed; dropping client connection\n"));
                return 0;
            }

            if (complete) {
                IPC_DispatchMsg(this, &mInMsg);
                mInMsg.Reset();
            }

            n -= nread;
            ptr += nread;
        }
    }
  
    if (inFlags & PR_POLL_WRITE) {
        LOG(("client socket is now writable\n"));

        if (mOutMsgQ.First())
            WriteMsgs(fd);
    }

    if (mOutMsgQ.First())
        outFlags |= PR_POLL_WRITE;

    return outFlags;
}




int
ipcClient::WriteMsgs(PRFileDesc *fd)
{
    while (mOutMsgQ.First()) {
        const char *buf = (const char *) mOutMsgQ.First()->MsgBuf();
        PRInt32 bufLen = (PRInt32) mOutMsgQ.First()->MsgLen();

        if (mSendOffset) {
            buf += mSendOffset;
            bufLen -= mSendOffset;
        }

        PRInt32 nw = PR_Write(fd, buf, bufLen);
        if (nw <= 0)
            break;

        LOG(("wrote %d bytes\n", nw));

        if (nw == bufLen) {
            mOutMsgQ.DeleteFirst();
            mSendOffset = 0;
        }
        else
            mSendOffset += nw;
    }

    return 0;
}

#endif
