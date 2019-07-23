




































#include <string.h>
#include "ipcm.h"
#include "pratom.h"

const nsID IPCM_TARGET =
{ 
    0x753ca8ff,
    0xc8c2,
    0x4601,
    {0xb1, 0x15, 0x8c, 0x29, 0x44, 0xda, 0x11, 0x50}
};

PRUint32
IPCM_NewRequestIndex()
{
    static PRInt32 sRequestIndex;
    return (PRUint32) PR_AtomicIncrement(&sRequestIndex);
}

#if 0




const PRUint32 ipcmMessagePing::MSG_TYPE = IPCM_MSG_TYPE_PING;
const PRUint32 ipcmMessageError::MSG_TYPE = IPCM_MSG_TYPE_ERROR;
const PRUint32 ipcmMessageClientHello::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_HELLO;
const PRUint32 ipcmMessageClientID::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_ID;
const PRUint32 ipcmMessageClientInfo::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_INFO;
const PRUint32 ipcmMessageClientAddName::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_ADD_NAME;
const PRUint32 ipcmMessageClientDelName::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_DEL_NAME;
const PRUint32 ipcmMessageClientAddTarget::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_ADD_TARGET;
const PRUint32 ipcmMessageClientDelTarget::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_DEL_TARGET;
const PRUint32 ipcmMessageQueryClientByName::MSG_TYPE = IPCM_MSG_TYPE_QUERY_CLIENT_BY_NAME;
const PRUint32 ipcmMessageQueryClientInfo::MSG_TYPE = IPCM_MSG_TYPE_QUERY_CLIENT_INFO;
const PRUint32 ipcmMessageForward::MSG_TYPE = IPCM_MSG_TYPE_FORWARD;
const PRUint32 ipcmMessageClientStatus::MSG_TYPE = IPCM_MSG_TYPE_CLIENT_STATUS;































struct ipcmClientInfoHeader
{
    PRUint32 mType;
    PRUint32 mID;
    PRUint32 mRequestIndex;
    PRUint16 mNameStart;
    PRUint16 mNameCount;
    PRUint16 mTargetStart;
    PRUint16 mTargetCount;
};

ipcmMessageClientInfo::ipcmMessageClientInfo(PRUint32 cID, PRUint32 rIdx, const char *names[], const nsID *targets[])
{
    ipcmClientInfoHeader hdr = {0};

    hdr.mType = MSG_TYPE;
    hdr.mID = cID;
    hdr.mRequestIndex = rIdx;
    hdr.mNameStart = sizeof(hdr);

    PRUint32 i, namesLen = 0;

    i = 0;
    while (names[i]) {
        namesLen += (strlen(names[i]) + 1);
        ++hdr.mNameCount;
        ++i;
    }

    i = 0;
    while (targets[i]) {
        ++hdr.mTargetCount;
        ++i;
    }

    
    
    
    hdr.mTargetStart = hdr.mNameStart + namesLen;

    
    
    
    PRUint32 dataLen = sizeof(hdr) + namesLen + hdr.mTargetCount * sizeof(nsID);

    Init(IPCM_TARGET, NULL, dataLen);
    
    
    
    
    SetData(0, (const char *) &hdr, sizeof(hdr));

    PRUint32 offset = sizeof(hdr);

    for (i = 0; names[i]; ++i) {
        PRUint32 len = strlen(names[i]) + 1;
        SetData(offset, names[i], len);
        offset += len;
    }

    for (i = 0; targets[i]; ++i) {
        PRUint32 len = sizeof(nsID);
        SetData(offset, (const char *) targets[i], len);
        offset += len;
    }
}

PRUint32
ipcmMessageClientInfo::ClientID() const
{
    ipcmClientInfoHeader *hdr = (ipcmClientInfoHeader *) Data();
    return hdr->mID;
}

PRUint32
ipcmMessageClientInfo::RequestIndex() const
{
    ipcmClientInfoHeader *hdr = (ipcmClientInfoHeader *) Data();
    return hdr->mRequestIndex;
}

PRUint32
ipcmMessageClientInfo::NameCount() const
{
    ipcmClientInfoHeader *hdr = (ipcmClientInfoHeader *) Data();
    return hdr->mNameCount;
}

PRUint32
ipcmMessageClientInfo::TargetCount() const
{
    ipcmClientInfoHeader *hdr = (ipcmClientInfoHeader *) Data();
    return hdr->mTargetCount;
}

const char *
ipcmMessageClientInfo::NextName(const char *name) const
{
    ipcmClientInfoHeader *hdr = (ipcmClientInfoHeader *) Data();

    if (!name)
        return (const char *) hdr + hdr->mNameStart;

    name += strlen(name) + 1;
    if (name == (const char *) hdr + hdr->mTargetStart)
        name = NULL;
    return name;
}

const nsID *
ipcmMessageClientInfo::NextTarget(const nsID *target) const
{
    ipcmClientInfoHeader *hdr = (ipcmClientInfoHeader *) Data();

    if (!target)
        return (const nsID *) (Data() + hdr->mTargetStart);

    if (++target == (const nsID *) (MsgBuf() + MsgLen()))
        target = NULL;
    return target;
}
#endif















ipcmMessageForward::ipcmMessageForward(PRUint32 type,
                                       PRUint32 cID,
                                       const nsID &target,
                                       const char *data,
                                       PRUint32 dataLen)
{
    int len = sizeof(ipcmMessageHeader) +  
              sizeof(cID) +                
              IPC_MSG_HEADER_SIZE +        
              dataLen;                     

    Init(IPCM_TARGET, NULL, len);

    ipcmMessageHeader ipcmHdr =
        { type, IPCM_NewRequestIndex() };

    SetData(0, (char *) &ipcmHdr, sizeof(ipcmHdr));
    SetData(sizeof(ipcmHdr), (char *) &cID, sizeof(cID));

    ipcMessageHeader hdr;
    hdr.mLen = IPC_MSG_HEADER_SIZE + dataLen;
    hdr.mVersion = IPC_MSG_VERSION;
    hdr.mFlags = 0;
    hdr.mTarget = target;

    SetData(sizeof(ipcmHdr) + sizeof(cID), (char *) &hdr, IPC_MSG_HEADER_SIZE);
    if (data)
        SetInnerData(0, data, dataLen);
}

void
ipcmMessageForward::SetInnerData(PRUint32 offset, const char *data, PRUint32 dataLen)
{
    SetData(sizeof(ipcmMessageHeader) + 4 + IPC_MSG_HEADER_SIZE + offset, data, dataLen);
}

PRUint32
ipcmMessageForward::ClientID() const
{
    return ((PRUint32 *) Data())[2];
}

const nsID &
ipcmMessageForward::InnerTarget() const
{
    ipcMessageHeader *hdr = (ipcMessageHeader *) (Data() + 12);
    return hdr->mTarget;
}

const char *
ipcmMessageForward::InnerData() const
{
    return Data() + 12 + IPC_MSG_HEADER_SIZE;
}

PRUint32
ipcmMessageForward::InnerDataLen() const
{
    ipcMessageHeader *hdr = (ipcMessageHeader *) (Data() + 12);
    return hdr->mLen - IPC_MSG_HEADER_SIZE;
}
