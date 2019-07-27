







#ifndef GrGLNameAllocator_DEFINED
#define GrGLNameAllocator_DEFINED

#include "SkRefCnt.h"
#include "gl/GrGLFunctions.h"






class GrGLNameAllocator {
public:
    










    GrGLNameAllocator(GrGLuint firstName, GrGLuint endName);

    



    ~GrGLNameAllocator();

    




    GrGLuint firstName() const { return fFirstName; }

    





    GrGLuint endName() const { return fEndName; }

    





    GrGLuint allocateName();

    









    void free(GrGLuint name);

private:
    class SparseNameRange;
    class SparseNameTree;
    class ContiguousNameRange;

    const GrGLuint fFirstName;
    const GrGLuint fEndName;
    SkAutoTUnref<SparseNameRange> fAllocatedNames;
};

#endif
