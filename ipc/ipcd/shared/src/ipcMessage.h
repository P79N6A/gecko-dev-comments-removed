




































#ifndef ipcMessage_h__
#define ipcMessage_h__

#include "nsID.h"



















struct ipcMessageHeader
{
    PRUint32 mLen;
    PRUint16 mVersion;
    PRUint16 mFlags;
    nsID     mTarget;
};

#define IPC_MSG_VERSION       (0x1)
#define IPC_MSG_HEADER_SIZE   (sizeof(ipcMessageHeader))
#define IPC_MSG_GUESSED_SIZE  (IPC_MSG_HEADER_SIZE + 64)







#define IPC_MSG_FLAG_SYNC_QUERY (0x1)
#define IPC_MSG_FLAG_SYNC_REPLY (0x2)





class ipcMessage
{
public:
    ipcMessage()
        : mNext(NULL)
        , mMetaData(0)
        , mMsgHdr(NULL)
        , mMsgOffset(0)
        , mMsgComplete(PR_FALSE)
        { }
    ipcMessage(const nsID &target, const char *data, PRUint32 dataLen)
        : mNext(NULL)
        , mMetaData(0)
        , mMsgHdr(NULL)
        , mMsgOffset(0)
        { Init(target, data, dataLen); }
   ~ipcMessage() NS_HIDDEN;

    
    
    
    NS_HIDDEN_(void) Reset();

    
    
    
    NS_HIDDEN_(ipcMessage *) Clone() const;

    
    
    
    
    
    
    
    
    NS_HIDDEN_(PRStatus) Init(const nsID &target, const char *data, PRUint32 dataLen);

    
    
    
    
    
    
    
    
    
    NS_HIDDEN_(PRStatus) SetData(PRUint32 offset, const char *data, PRUint32 dataLen);

    
    
    
    void SetFlag(PRUint16 flag)          { mMsgHdr->mFlags |= flag; }
    void ClearFlag(PRUint16 flag)        { mMsgHdr->mFlags &= ~flag; }
    PRBool TestFlag(PRUint16 flag) const { return mMsgHdr->mFlags & flag; }

    
    
    
    
    PRBool IsComplete() const { return mMsgComplete; }

    
    
    
    const ipcMessageHeader *Header()  const { return mMsgHdr; }
    const nsID             &Target()  const { return mMsgHdr->mTarget; }
    const char             *Data()    const { return (char *) mMsgHdr + IPC_MSG_HEADER_SIZE; }
    PRUint32                DataLen() const { return mMsgHdr->mLen - IPC_MSG_HEADER_SIZE; }
    const char             *MsgBuf()  const { return (char *) mMsgHdr; }
    PRUint32                MsgLen()  const { return mMsgHdr->mLen; }

    
    
    
    
    
    
    
    
    NS_HIDDEN_(PRBool) Equals(const nsID &target, const char *data, PRUint32 dataLen) const;
    NS_HIDDEN_(PRBool) Equals(const ipcMessage *msg) const;

    
    
    
    
    NS_HIDDEN_(PRStatus) WriteTo(char     *buf,
                                 PRUint32  bufLen,
                                 PRUint32 *bytesWritten,
                                 PRBool   *complete);

    
    
    
    
    NS_HIDDEN_(PRStatus) ReadFrom(const char *buf,
                                  PRUint32    bufLen,
                                  PRUint32   *bytesRead,
                                  PRBool     *complete);

    
    
    
    class ipcMessage *mNext;

    
    
    
    
    
    PRUint32          mMetaData;

private:
    ipcMessageHeader *mMsgHdr;

    
    PRUint32          mMsgOffset;
    PRPackedBool      mMsgComplete;
};

#endif 
