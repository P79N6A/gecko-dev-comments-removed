








#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include "SkAutoCoInitialize.h"

SkAutoCoInitialize::SkAutoCoInitialize() :
    fHR(
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)
    )
{ }

SkAutoCoInitialize::~SkAutoCoInitialize() {
    if (SUCCEEDED(this->fHR)) {
        CoUninitialize();
    }
}

bool SkAutoCoInitialize::succeeded() {
    return SUCCEEDED(this->fHR) || RPC_E_CHANGED_MODE == this->fHR;
}
