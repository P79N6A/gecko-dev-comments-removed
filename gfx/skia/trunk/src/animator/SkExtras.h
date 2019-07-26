








#ifndef SkExtras_DEFINED
#define SkExtras_DEFINED

#include "SkScript.h"

class SkExtras {
public:
            SkExtras();
    virtual ~SkExtras() {}

    virtual SkDisplayable* createInstance(SkDisplayTypes type) = 0;
    virtual bool definesType(SkDisplayTypes type) = 0;
#if SK_USE_CONDENSED_INFO == 0
    virtual const SkMemberInfo* getMembers(SkDisplayTypes type, int* infoCountPtr) = 0;
#endif
#ifdef SK_DEBUG
    virtual const char* getName(SkDisplayTypes type) = 0;
#endif
    virtual SkDisplayTypes getType(const char match[], size_t len ) = 0;

    SkScriptEngine::_propertyCallBack fExtraCallBack;
    void* fExtraStorage;
};

#endif 
