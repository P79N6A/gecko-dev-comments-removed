








#ifndef SkAutoCo_DEFINED
#define SkAutoCo_DEFINED

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SkTemplates.h"





class SkAutoCoInitialize : SkNoncopyable {
private:
    HRESULT fHR;
public:
    SkAutoCoInitialize();
    ~SkAutoCoInitialize();
    bool succeeded();
};

#endif
