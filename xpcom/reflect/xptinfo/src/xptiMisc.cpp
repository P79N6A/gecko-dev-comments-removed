








































#include "xptiprivate.h"

struct xptiFileTypeEntry
{
    const char*         name;
    int                 len;
    xptiFileType::Type  type;
};

static const xptiFileTypeEntry g_Entries[] = 
    {
        {".xpt", 4, xptiFileType::XPT},            
        {".zip", 4, xptiFileType::ZIP},            
        {".jar", 4, xptiFileType::ZIP},            
        {nsnull, 0, xptiFileType::UNKNOWN}            
    };


xptiFileType::Type xptiFileType::GetType(const nsACString& aType)
{
    for(const xptiFileTypeEntry* p = g_Entries; p->name; p++)
    {
        if (StringEndsWith(aType, nsDependentCString(p->name, p->len)))
            return p->type;
    }
    return UNKNOWN;        
}        
