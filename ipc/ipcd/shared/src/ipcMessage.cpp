




































#include <stdlib.h>
#include <string.h>
#include "prlog.h"
#include "ipcMessage.h"

ipcMessage::~ipcMessage()
{
    if (mMsgHdr)
        free(mMsgHdr);
}

void
ipcMessage::Reset()
{
    if (mMsgHdr) {
        free(mMsgHdr);
        mMsgHdr = NULL;
    }

    mMsgOffset = 0;
    mMsgComplete = PR_FALSE;
}

ipcMessage *
ipcMessage::Clone() const
{
    ipcMessage *clone = new ipcMessage();
    if (!clone)
        return NULL;

    
    if (mMsgHdr) {
        clone->mMsgHdr = (ipcMessageHeader *) malloc(mMsgHdr->mLen);
        memcpy(clone->mMsgHdr, mMsgHdr, mMsgHdr->mLen);
    }
    else
        clone->mMsgHdr = NULL;

    clone->mMsgOffset = mMsgOffset;
    clone->mMsgComplete = mMsgComplete;

    return clone;
}

PRStatus
ipcMessage::Init(const nsID &target, const char *data, PRUint32 dataLen)
{
    if (mMsgHdr)
        free(mMsgHdr);
    mMsgComplete = PR_FALSE;

    
    PRUint32 msgLen = IPC_MSG_HEADER_SIZE + dataLen;
    mMsgHdr = (ipcMessageHeader *) malloc(msgLen);
    if (!mMsgHdr) {
        mMsgHdr = NULL;
        return PR_FAILURE;
    }

    
    mMsgHdr->mLen = msgLen;
    mMsgHdr->mVersion = IPC_MSG_VERSION;
    mMsgHdr->mFlags = 0;
    mMsgHdr->mTarget = target;

    if (data)
        SetData(0, data, dataLen);

    mMsgComplete = PR_TRUE;
    return PR_SUCCESS;
}

PRStatus
ipcMessage::SetData(PRUint32 offset, const char *data, PRUint32 dataLen)
{
    PR_ASSERT(mMsgHdr != NULL);

    if (offset + dataLen > DataLen())
        return PR_FAILURE;

    memcpy((char *) Data() + offset, data, dataLen);
    return PR_SUCCESS;
}

PRBool
ipcMessage::Equals(const nsID &target, const char *data, PRUint32 dataLen) const
{
    return mMsgComplete && 
           mMsgHdr->mTarget.Equals(target) &&
           DataLen() == dataLen &&
           memcmp(Data(), data, dataLen) == 0;
}

PRBool
ipcMessage::Equals(const ipcMessage *msg) const
{
    PRUint32 msgLen = MsgLen();
    return mMsgComplete && msg->mMsgComplete &&
           msgLen == msg->MsgLen() &&
           memcmp(MsgBuf(), msg->MsgBuf(), msgLen) == 0;
}

PRStatus
ipcMessage::WriteTo(char     *buf,
                    PRUint32  bufLen,
                    PRUint32 *bytesWritten,
                    PRBool   *complete)
{
    if (!mMsgComplete)
        return PR_FAILURE;

    if (mMsgOffset == MsgLen()) {
        *bytesWritten = 0;
        *complete = PR_TRUE;
        return PR_SUCCESS;
    }

    PRUint32 count = MsgLen() - mMsgOffset;
    if (count > bufLen)
        count = bufLen;

    memcpy(buf, MsgBuf() + mMsgOffset, count);
    mMsgOffset += count;

    *bytesWritten = count;
    *complete = (mMsgOffset == MsgLen());

    return PR_SUCCESS;
}

PRStatus
ipcMessage::ReadFrom(const char *buf,
                     PRUint32    bufLen,
                     PRUint32   *bytesRead,
                     PRBool     *complete)
{
    *bytesRead = 0;

    if (mMsgComplete) {
        *complete = PR_TRUE;
        return PR_SUCCESS;
    }

    if (mMsgHdr) {
        
        if (mMsgOffset < sizeof(PRUint32)) {
            
            if (mMsgOffset + bufLen < sizeof(PRUint32)) {
                
                memcpy((char *) mMsgHdr + mMsgOffset, buf, bufLen);
                mMsgOffset += bufLen;
                *bytesRead = bufLen;
                *complete = PR_FALSE;
                return PR_SUCCESS;
            }
            else {
                
                PRUint32 count = sizeof(PRUint32) - mMsgOffset;
                memcpy((char *) MsgBuf() + mMsgOffset, buf, count);
                mMsgOffset += count;
                buf += count;
                bufLen -= count;
                *bytesRead = count;
                
                if (MsgLen() > IPC_MSG_GUESSED_SIZE) {
                    
                    mMsgHdr = (ipcMessageHeader *) realloc(mMsgHdr, MsgLen());
                }
            }
        }
    }
    else {
        if (bufLen < sizeof(PRUint32)) {
            
            
            PRUint32 msgLen = IPC_MSG_GUESSED_SIZE;
            mMsgHdr = (ipcMessageHeader *) malloc(msgLen);
            if (!mMsgHdr)
                return PR_FAILURE;
            memcpy(mMsgHdr, buf, bufLen);
            mMsgOffset = bufLen;
            *bytesRead = bufLen;
            *complete = PR_FALSE;
            return PR_SUCCESS;
        }
        else {
            PRUint32 msgLen = *(PRUint32 *) buf;
            mMsgHdr = (ipcMessageHeader *) malloc(msgLen);
            if (!mMsgHdr)
                return PR_FAILURE;
            mMsgHdr->mLen = msgLen;
            mMsgOffset = 0;
        }
    }

    

    PRUint32 count = MsgLen() - mMsgOffset;
    if (count > bufLen)
        count = bufLen;

    memcpy((char *) mMsgHdr + mMsgOffset, buf, count);
    mMsgOffset += count;
    *bytesRead += count;

    *complete = mMsgComplete = (mMsgOffset == MsgLen());
    return PR_SUCCESS;
}
