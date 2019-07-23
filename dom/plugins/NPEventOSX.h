





































#ifndef mozilla_dom_plugins_NPEventOSX_h
#define mozilla_dom_plugins_NPEventOSX_h 1


#include "npapi.h"

#warning This is only a stub implementation IMPLEMENT ME

template <>
struct ParamTraits<NPEvent>
{
    typedef NPEvent paramType;

    static void Write(Message* aMsg, const paramType& aParam)
    {
    }

    static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
    {
        return true;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
    }
};


#endif 
