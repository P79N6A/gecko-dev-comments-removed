






#ifndef GrFakeRefObj_DEFINED
#define GrFakeRefObj_DEFINED

#include "SkTypes.h"
#include "gl/GrGLInterface.h"








class GrFakeRefObj : SkNoncopyable {
public:
    GrFakeRefObj()
        : fRef(0)
        , fHighRefCount(0)
        , fMarkedForDeletion(false)
        , fDeleted(false) {

        
        static int fNextID = 0;

        fID = ++fNextID;
    }
    virtual ~GrFakeRefObj() {};

    void ref() {
        fRef++;
        if (fHighRefCount < fRef) {
            fHighRefCount = fRef;
        }
    }
    void unref() {
        fRef--;
        GrAlwaysAssert(fRef >= 0);

        
        
        
        if (0 == fRef && fMarkedForDeletion) {
            this->deleteAction();
        }
    }
    int getRefCount() const             { return fRef; }
    int getHighRefCount() const         { return fHighRefCount; }

    GrGLuint getID() const              { return fID; }

    void setMarkedForDeletion()         { fMarkedForDeletion = true; }
    bool getMarkedForDeletion() const   { return fMarkedForDeletion; }

    bool getDeleted() const             { return fDeleted; }

    
    
    virtual void deleteAction() {
        this->setDeleted();
    }

protected:
private:
    int         fRef;               
    int         fHighRefCount;      
    GrGLuint    fID;                
    bool        fMarkedForDeletion;
    
    
    bool        fDeleted;

    
    void setDeleted()                   { fDeleted = true; }
};






#define GR_DEFINE_CREATOR(className)                        \
    public:                                                 \
    static GrFakeRefObj *create ## className() {            \
        return SkNEW(className);                            \
    }

#endif 
