




































#ifndef ipcMessageUtils_h__
#define ipcMessageUtils_h__

class ipcMessage;












template<class T>
class ipcMessageCast
{
public:
    ipcMessageCast() : mPtr(NULL) {}
    ipcMessageCast(const ipcMessage *ptr) : mPtr((const T *) ptr) {}
    void operator=(const ipcMessage *ptr) { mPtr = (const T *) ptr; }
    const T *operator->() { return mPtr; }
private:
    const T *mPtr;
};

#endif 
