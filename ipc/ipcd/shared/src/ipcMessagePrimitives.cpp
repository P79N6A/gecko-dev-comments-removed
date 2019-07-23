




































#include <string.h>
#include "ipcMessagePrimitives.h"

ipcMessage_DWORD_STR::ipcMessage_DWORD_STR(const nsID &target,
                                           PRUint32    first,
                                           const char *second)
{
    int sLen = strlen(second);
    Init(target, NULL, 4 + sLen + 1);
    SetData(0, (char *) &first, 4);
    SetData(4, second, sLen + 1);
}

ipcMessage_DWORD_DWORD_STR::ipcMessage_DWORD_DWORD_STR(const nsID &target,
                                                       PRUint32    first,
                                                       PRUint32    second,
                                                       const char *third)
{
    int sLen = strlen(third);
    Init(target, NULL, 8 + sLen + 1);
    SetData(0, (char *) &first, 4);
    SetData(4, (char *) &second, 4);
    SetData(8, third, sLen + 1);
}

ipcMessage_DWORD_ID::ipcMessage_DWORD_ID(const nsID &target,
                                         PRUint32    first,
                                         const nsID &second)
{
    Init(target, NULL, 4 + sizeof(nsID));
    SetData(0, (char *) &first, 4);
    SetData(4, (char *) &second, sizeof(nsID));
}

ipcMessage_DWORD_DWORD_ID::ipcMessage_DWORD_DWORD_ID(const nsID &target,
                                                     PRUint32    first,
                                                     PRUint32    second,
                                                     const nsID &third)
{
    Init(target, NULL, 8 + sizeof(nsID));
    SetData(0, (char *) &first, 4);
    SetData(4, (char *) &second, 4);
    SetData(8, (char *) &third, sizeof(nsID));
}
