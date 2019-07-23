




































#ifndef SWITCHTOUITHREAD_H
#define SWITCHTOUITHREAD_H



struct MethodInfo;





class nsSwitchToUIThread {

public:
    virtual BOOL CallMethod(MethodInfo *info) = 0;

};





struct MethodInfo {
    nsSwitchToUIThread* target;
    UINT        methodId;
    int         nArgs;
    DWORD*      args;

    MethodInfo(nsSwitchToUIThread *obj, UINT id, int numArgs=0, DWORD *arguments = 0) {
        target   = obj;
        methodId = id;
        nArgs    = numArgs;
        args     = arguments;
    }
    
    BOOL Invoke() { return target->CallMethod(this); }
};

#endif 

