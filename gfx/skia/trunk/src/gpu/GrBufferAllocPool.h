






#ifndef GrBufferAllocPool_DEFINED
#define GrBufferAllocPool_DEFINED

#include "SkTArray.h"
#include "SkTDArray.h"
#include "SkTypes.h"

class GrGeometryBuffer;
class GrGpu;













class GrBufferAllocPool : SkNoncopyable {
public:
    



    void unmap();

    


    void reset();

    


    int preallocatedBuffersRemaining() const;

    


    int preallocatedBufferCount() const;

    


    void putBack(size_t bytes);

    


    GrGpu* getGpu() { return fGpu; }

protected:
    




    enum BufferType {
        kVertex_BufferType,
        kIndex_BufferType,
    };

    















     GrBufferAllocPool(GrGpu* gpu,
                       BufferType bufferType,
                       bool frequentResetHint,
                       size_t   bufferSize = 0,
                       int preallocBufferCnt = 0);

    virtual ~GrBufferAllocPool();

    




    size_t preallocatedBufferSize() const {
        return fPreallocBuffers.count() ? fMinBlockSize : 0;
    }

    


















    void* makeSpace(size_t size,
                    size_t alignment,
                    const GrGeometryBuffer** buffer,
                    size_t* offset);

    









    int currentBufferItems(size_t itemSize) const;

    GrGeometryBuffer* createBuffer(size_t size);

private:

    
    friend class GrGpu;
    void releaseGpuRef();

    struct BufferBlock {
        size_t              fBytesFree;
        GrGeometryBuffer*   fBuffer;
    };

    bool createBlock(size_t requestSize);
    void destroyBlock();
    void flushCpuData(const BufferBlock& block, size_t flushSize);
#ifdef SK_DEBUG
    void validate(bool unusedBlockAllowed = false) const;
#endif

    size_t                          fBytesInUse;

    GrGpu*                          fGpu;
    bool                            fGpuIsReffed;
    bool                            fFrequentResetHint;
    SkTDArray<GrGeometryBuffer*>    fPreallocBuffers;
    size_t                          fMinBlockSize;
    BufferType                      fBufferType;

    SkTArray<BufferBlock>           fBlocks;
    int                             fPreallocBuffersInUse;
    
    
    int                             fPreallocBufferStartIdx;
    SkAutoMalloc                    fCpuData;
    void*                           fBufferPtr;
};

class GrVertexBuffer;




class GrVertexBufferAllocPool : public GrBufferAllocPool {
public:
    













    GrVertexBufferAllocPool(GrGpu* gpu,
                            bool frequentResetHint,
                            size_t bufferSize = 0,
                            int preallocBufferCnt = 0);

    




















    void* makeSpace(size_t vertexSize,
                    int vertexCount,
                    const GrVertexBuffer** buffer,
                    int* startVertex);

    


    bool appendVertices(size_t vertexSize,
                        int vertexCount,
                        const void* vertices,
                        const GrVertexBuffer** buffer,
                        int* startVertex);

    









    int currentBufferVertices(size_t vertexSize) const;

    








    int preallocatedBufferVertices(size_t vertexSize) const;

private:
    typedef GrBufferAllocPool INHERITED;
};

class GrIndexBuffer;




class GrIndexBufferAllocPool : public GrBufferAllocPool {
public:
    













    GrIndexBufferAllocPool(GrGpu* gpu,
                           bool frequentResetHint,
                           size_t bufferSize = 0,
                           int preallocBufferCnt = 0);

    

















    void* makeSpace(int indexCount,
                    const GrIndexBuffer** buffer,
                    int* startIndex);

    


    bool appendIndices(int indexCount,
                       const void* indices,
                       const GrIndexBuffer** buffer,
                       int* startIndex);

    






    int currentBufferIndices() const;

    






    int preallocatedBufferIndices() const;

private:
    typedef GrBufferAllocPool INHERITED;
};

#endif
