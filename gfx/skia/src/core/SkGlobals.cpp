








#include "SkGlobals.h"
#include "SkThread.h"

SkGlobals::Rec::~Rec()
{
}

SkGlobals::Rec* SkGlobals::Find(uint32_t tag, Rec* (*create_proc)())
{
    SkGlobals::BootStrap&   bootstrap = SkGlobals::GetBootStrap();

    Rec* rec = bootstrap.fHead;
    while (rec)
    {
        if (rec->fTag == tag)
            return rec;
        rec = rec->fNext;
    }

    if (create_proc == NULL) 
        return NULL;

    
    

    bootstrap.fMutex.acquire();

    
    

    Rec*& head = bootstrap.fHead;
    rec = head;
    while (rec)
    {
        if (rec->fTag == tag)
            break;
        rec = rec->fNext;
    }

    if (rec == NULL && (rec = create_proc()) != NULL)
    {
        rec->fTag = tag;
        rec->fNext = head;
        bootstrap.fHead = rec;
    }

    bootstrap.fMutex.release();
    return rec;
}

void SkGlobals::Init()
{
}

void SkGlobals::Term()
{
    SkGlobals::BootStrap&   bootstrap = SkGlobals::GetBootStrap();

    bootstrap.fMutex.acquire();

    Rec*&   head = bootstrap.fHead;
    Rec*    rec = head;

    while (rec)
    {
        Rec* next = rec->fNext;
        SkDELETE(rec);
        rec = next;
    }

    bootstrap.fHead = NULL;
    bootstrap.fMutex.release();
}


