




































#ifndef ipcMessagePrimitives_h__
#define ipcMessagePrimitives_h__

#include "ipcMessage.h"

class ipcMessage_DWORD : public ipcMessage
{
public:
    ipcMessage_DWORD(const nsID &target, PRUint32 first)
    {
        Init(target, (char *) &first, sizeof(first));
    }

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }
};

class ipcMessage_DWORD_DWORD : public ipcMessage
{
public:
    ipcMessage_DWORD_DWORD(const nsID &target, PRUint32 first, PRUint32 second)
    {
        PRUint32 data[2] = { first, second };
        Init(target, (char *) data, sizeof(data));
    }

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }

    PRUint32 Second() const
    {
        return ((PRUint32 *) Data())[1];
    }
};

class ipcMessage_DWORD_DWORD_DWORD : public ipcMessage
{
public:
    ipcMessage_DWORD_DWORD_DWORD(const nsID &target, PRUint32 first, PRUint32 second, PRUint32 third)
    {
        PRUint32 data[3] = { first, second, third };
        Init(target, (char *) data, sizeof(data));
    }

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }

    PRUint32 Second() const
    {
        return ((PRUint32 *) Data())[1];
    }

    PRUint32 Third() const
    {
        return ((PRUint32 *) Data())[2];
    }
};

class ipcMessage_DWORD_DWORD_DWORD_DWORD : public ipcMessage
{
public:
    ipcMessage_DWORD_DWORD_DWORD_DWORD(const nsID &target, PRUint32 first, PRUint32 second, PRUint32 third, PRUint32 fourth)
    {
        PRUint32 data[4] = { first, second, third, fourth };
        Init(target, (char *) data, sizeof(data));
    }

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }

    PRUint32 Second() const
    {
        return ((PRUint32 *) Data())[1];
    }

    PRUint32 Third() const
    {
        return ((PRUint32 *) Data())[2];
    }

    PRUint32 Fourth() const
    {
        return ((PRUint32 *) Data())[3];
    }
};

class ipcMessage_DWORD_STR : public ipcMessage
{
public:
    ipcMessage_DWORD_STR(const nsID &target, PRUint32 first, const char *second) NS_HIDDEN;

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }

    const char *Second() const
    {
        return Data() + sizeof(PRUint32);
    }
};

class ipcMessage_DWORD_DWORD_STR : public ipcMessage
{
public:
    ipcMessage_DWORD_DWORD_STR(const nsID &target, PRUint32 first, PRUint32 second, const char *third) NS_HIDDEN;

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }

    PRUint32 Second() const
    {
        return ((PRUint32 *) Data())[1];
    }

    const char *Third() const
    {
        return Data() + 2 * sizeof(PRUint32);
    }
};

class ipcMessage_DWORD_ID : public ipcMessage
{
public:
    ipcMessage_DWORD_ID(const nsID &target, PRUint32 first, const nsID &second) NS_HIDDEN;

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }

    const nsID &Second() const
    {
        return * (const nsID *) (Data() + sizeof(PRUint32));
    }
};

class ipcMessage_DWORD_DWORD_ID : public ipcMessage
{
public:
    ipcMessage_DWORD_DWORD_ID(const nsID &target, PRUint32 first, PRUint32 second, const nsID &third) NS_HIDDEN;

    PRUint32 First() const
    {
        return ((PRUint32 *) Data())[0];
    }

    PRUint32 Second() const
    {
        return ((PRUint32 *) Data())[1];
    }

    const nsID &Third() const
    {
        return * (const nsID *) (Data() + 2 * sizeof(PRUint32));
    }
};

#endif 
