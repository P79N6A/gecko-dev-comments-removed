




































#ifndef ipcm_h__
#define ipcm_h__

#include "ipcMessage.h"
#include "ipcMessagePrimitives.h"








extern const nsID IPCM_TARGET;
































#define IPCM_MSG_CLASS_REQ (1 << 24)
#define IPCM_MSG_CLASS_ACK (2 << 24)
#define IPCM_MSG_CLASS_PSH (4 << 24)


#define IPCM_MSG_REQ_PING                   (IPCM_MSG_CLASS_REQ | 1)
#define IPCM_MSG_REQ_FORWARD                (IPCM_MSG_CLASS_REQ | 2)
#define IPCM_MSG_REQ_CLIENT_HELLO           (IPCM_MSG_CLASS_REQ | 3)
#define IPCM_MSG_REQ_CLIENT_ADD_NAME        (IPCM_MSG_CLASS_REQ | 4)
#define IPCM_MSG_REQ_CLIENT_DEL_NAME        (IPCM_MSG_CLASS_REQ | 5)
#define IPCM_MSG_REQ_CLIENT_ADD_TARGET      (IPCM_MSG_CLASS_REQ | 6)
#define IPCM_MSG_REQ_CLIENT_DEL_TARGET      (IPCM_MSG_CLASS_REQ | 7)
#define IPCM_MSG_REQ_QUERY_CLIENT_BY_NAME   (IPCM_MSG_CLASS_REQ | 8)
#define IPCM_MSG_REQ_QUERY_CLIENT_NAMES     (IPCM_MSG_CLASS_REQ | 9)  // TODO
#define IPCM_MSG_REQ_QUERY_CLIENT_TARGETS   (IPCM_MSG_CLASS_REQ | 10) // TODO


#define IPCM_MSG_ACK_RESULT                 (IPCM_MSG_CLASS_ACK | 1)
#define IPCM_MSG_ACK_CLIENT_ID              (IPCM_MSG_CLASS_ACK | 2)
#define IPCM_MSG_ACK_CLIENT_NAMES           (IPCM_MSG_CLASS_ACK | 3)  // TODO
#define IPCM_MSG_ACK_CLIENT_TARGETS         (IPCM_MSG_CLASS_ACK | 4)  // TODO


#define IPCM_MSG_PSH_CLIENT_STATE           (IPCM_MSG_CLASS_PSH | 1)
#define IPCM_MSG_PSH_FORWARD                (IPCM_MSG_CLASS_PSH | 2)






struct ipcmMessageHeader
{
    PRUint32 mType;
    PRUint32 mRequestIndex;
};




static inline int
IPCM_GetType(const ipcMessage *msg)
{
    return ((const ipcmMessageHeader *) msg->Data())->mType;
}




static inline PRUint32
IPCM_GetRequestIndex(const ipcMessage *msg)
{
    return ((const ipcmMessageHeader *) msg->Data())->mRequestIndex;
}




NS_HIDDEN_(PRUint32)
IPCM_NewRequestIndex();


































































































































#define IPCM_OK               0  // success: generic
#define IPCM_ERROR_GENERIC   -1  // failure: generic
#define IPCM_ERROR_NO_CLIENT -2  // failure: client does not exist



























#define IPCM_CLIENT_STATE_UP     1
#define IPCM_CLIENT_STATE_DOWN   2






































class ipcmMessagePing : public ipcMessage_DWORD_DWORD
{
public:
    ipcmMessagePing()
        : ipcMessage_DWORD_DWORD(
            IPCM_TARGET,
            IPCM_MSG_REQ_PING,
            IPCM_NewRequestIndex()) {}
};

class ipcmMessageForward : public ipcMessage
{
public:
    
    
    
    
    
    ipcmMessageForward(PRUint32 type,
                       PRUint32 clientID,
                       const nsID &target,
                       const char *data,
                       PRUint32 dataLen) NS_HIDDEN;

    
    
    NS_HIDDEN_(void) SetInnerData(PRUint32 offset, const char *data, PRUint32 dataLen);

    NS_HIDDEN_(PRUint32)     ClientID() const;
    NS_HIDDEN_(const nsID &) InnerTarget() const;
    NS_HIDDEN_(const char *) InnerData() const;
    NS_HIDDEN_(PRUint32)     InnerDataLen() const;
};

class ipcmMessageClientHello : public ipcMessage_DWORD_DWORD
{
public:
    ipcmMessageClientHello()
        : ipcMessage_DWORD_DWORD(
            IPCM_TARGET,
            IPCM_MSG_REQ_CLIENT_HELLO,
            IPCM_NewRequestIndex()) {}
};

class ipcmMessageClientAddName : public ipcMessage_DWORD_DWORD_STR
{
public:
    ipcmMessageClientAddName(const char *name)
        : ipcMessage_DWORD_DWORD_STR(
            IPCM_TARGET,
            IPCM_MSG_REQ_CLIENT_ADD_NAME,
            IPCM_NewRequestIndex(),
            name) {}

    const char *Name() const { return Third(); }
};

class ipcmMessageClientDelName : public ipcMessage_DWORD_DWORD_STR
{
public:
    ipcmMessageClientDelName(const char *name)
        : ipcMessage_DWORD_DWORD_STR(
            IPCM_TARGET,
            IPCM_MSG_REQ_CLIENT_DEL_NAME,
            IPCM_NewRequestIndex(),
            name) {}

    const char *Name() const { return Third(); }
};

class ipcmMessageClientAddTarget : public ipcMessage_DWORD_DWORD_ID
{
public:
    ipcmMessageClientAddTarget(const nsID &target)
        : ipcMessage_DWORD_DWORD_ID(
            IPCM_TARGET,
            IPCM_MSG_REQ_CLIENT_ADD_TARGET,
            IPCM_NewRequestIndex(),
            target) {}

    const nsID &Target() const { return Third(); }
};

class ipcmMessageClientDelTarget : public ipcMessage_DWORD_DWORD_ID
{
public:
    ipcmMessageClientDelTarget(const nsID &target)
        : ipcMessage_DWORD_DWORD_ID(
            IPCM_TARGET,
            IPCM_MSG_REQ_CLIENT_ADD_TARGET,
            IPCM_NewRequestIndex(),
            target) {}

    const nsID &Target() const { return Third(); }
};

class ipcmMessageQueryClientByName : public ipcMessage_DWORD_DWORD_STR
{
public:
    ipcmMessageQueryClientByName(const char *name)
        : ipcMessage_DWORD_DWORD_STR(
            IPCM_TARGET,
            IPCM_MSG_REQ_QUERY_CLIENT_BY_NAME,
            IPCM_NewRequestIndex(),
            name) {}

    const char *Name() const { return Third(); }
    PRUint32 RequestIndex() const { return Second(); }
};



class ipcmMessageResult : public ipcMessage_DWORD_DWORD_DWORD
{
public:
    ipcmMessageResult(PRUint32 requestIndex, PRInt32 status)
        : ipcMessage_DWORD_DWORD_DWORD(
            IPCM_TARGET,
            IPCM_MSG_ACK_RESULT,
            requestIndex,
            (PRUint32) status) {}

    PRInt32 Status() const { return (PRInt32) Third(); }
};

class ipcmMessageClientID : public ipcMessage_DWORD_DWORD_DWORD
{
public:
    ipcmMessageClientID(PRUint32 requestIndex, PRUint32 clientID)
        : ipcMessage_DWORD_DWORD_DWORD(
            IPCM_TARGET,
            IPCM_MSG_ACK_CLIENT_ID,
            requestIndex,
            clientID) {}

    PRUint32 ClientID() const { return Third(); }
};



class ipcmMessageClientState : public ipcMessage_DWORD_DWORD_DWORD_DWORD
{
public:
    ipcmMessageClientState(PRUint32 clientID, PRUint32 clientStatus)
        : ipcMessage_DWORD_DWORD_DWORD_DWORD(
            IPCM_TARGET,
            IPCM_MSG_PSH_CLIENT_STATE,
            0,
            clientID,
            clientStatus) {}

    PRUint32 ClientID() const { return Third(); }
    PRUint32 ClientState() const { return Fourth(); }
};

#endif 
