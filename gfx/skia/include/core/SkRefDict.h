








#ifndef SkRefDict_DEFINED
#define SkRefDict_DEFINED

#include "SkRefCnt.h"





class SK_API SkRefDict : SkNoncopyable {
public:
    SkRefDict();
    ~SkRefDict();

    



    SkRefCnt* find(const char name[]) const;
    
    






    void set(const char name[], SkRefCnt* data);

    


    void remove(const char name[]) { this->set(name, NULL); }

    


    void removeAll();

private:
    struct Impl;
    Impl* fImpl;
};

#endif
