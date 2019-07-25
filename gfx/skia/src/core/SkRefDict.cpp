








#include "SkRefDict.h"
#include "SkString.h"

struct SkRefDict::Impl {
    Impl*       fNext;
    SkString    fName;
    SkRefCnt*   fData;
};

SkRefDict::SkRefDict() : fImpl(NULL) {}

SkRefDict::~SkRefDict() {
    this->removeAll();
}

SkRefCnt* SkRefDict::find(const char name[]) const {
    if (NULL == name) {
        return NULL;
    }

    Impl* rec = fImpl;
    while (rec) {
        if (rec->fName.equals(name)) {
            return rec->fData;
        }
        rec = rec->fNext;
    }
    return NULL;
}

void SkRefDict::set(const char name[], SkRefCnt* data) {
    if (NULL == name) {
        return;
    }

    Impl* rec = fImpl;
    Impl* prev = NULL;
    while (rec) {
        if (rec->fName.equals(name)) {
            if (data) {
                
                data->ref();
                rec->fData->unref();
                rec->fData = data;
            } else {
                
                rec->fData->unref();
                if (prev) {
                    prev->fNext = rec->fNext;
                } else {
                    fImpl = rec->fNext;
                }
            }
            return;
        }
        prev = rec;
        rec = rec->fNext;
    }

    
    data->ref();
    rec = new Impl;
    rec->fName.set(name);
    rec->fData = data;
    
    rec->fNext = fImpl;
    fImpl = rec;
}

void SkRefDict::removeAll() {
    Impl* rec = fImpl;
    while (rec) {
        Impl* next = rec->fNext;
        rec->fData->unref();
        delete rec;
        rec = next;
    }
    fImpl = NULL;
}

