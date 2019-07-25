








































#ifndef jsion_ion_assembler_buffer_with_constant_pools_h__
#define jsion_ion_assembler_buffer_with_constant_pools_h__
#include "ion/shared/IonAssemblerBuffer.h"
#include "assembler/wtf/SegmentedVector.h"

namespace js {
namespace ion {

typedef SegmentedVector<BufferOffset, 512> LoadOffsets;

struct Pool {
    const int maxOffset;
    const int immSize;
    const int instSize;
    const int bias;
    const int alignment;
    const bool isBackref;
    const bool canDedup;
    Pool *other;
    uint8 *poolData;
    uint32 numEntries;
    uint32 buffSize;
    LoadOffsets loadOffsets;

    
    
    
    
    
    
    
    

    BufferOffset limitingUser;
    int limitingUsee;

    Pool(int maxOffset_, int immSize_, int instSize_, int bias_, int alignment_,
         bool isBackref_ = false, bool canDedup_ = false, Pool *other_ = NULL)
        : maxOffset(maxOffset_), immSize(immSize_), instSize(instSize),
          bias(bias_), alignment(alignment_),
          isBackref(isBackref_), canDedup(canDedup_), other(other_),
          poolData(static_cast<uint8 *>(malloc(8*immSize))), numEntries(0),
          buffSize(8), limitingUser(), limitingUsee(INT_MAX)
    {
    }
    static const int garbage=0xa5a5a5a5;
    Pool() : maxOffset(garbage), immSize(garbage), instSize(garbage), bias(garbage),
             alignment(garbage), isBackref(garbage), canDedup(garbage)
    {
    }
    
    
    void updateLimiter(BufferOffset nextInst) {
        int oldRange, newRange;
        if (isBackref) {
            
            oldRange = limitingUser.getOffset() - ((numEntries - limitingUsee) * immSize);
            newRange = nextInst.getOffset();
        } else {
            oldRange = (limitingUsee * immSize) - limitingUser.getOffset();
            newRange = (numEntries * immSize) - nextInst.getOffset();
        }
        if (!limitingUser.assigned() || newRange > oldRange) {
            
            limitingUser = nextInst;
            limitingUsee = numEntries;
        }
    }
    
    
    
    

    
    
    
    
    bool checkFullBackref(int poolOffset, int codeOffset) {
        if (signed(limitingUser.getOffset() + bias - codeOffset + poolOffset + (numEntries - limitingUsee) * immSize) >= maxOffset) {
            return true;
        }
        return false;
    }

    
    
    
    bool checkFull(int poolOffset) {
        
        
        
        JS_ASSERT(!isBackref);

        
        
        if (poolOffset + limitingUsee * immSize - (limitingUser.getOffset() + bias) > maxOffset) {
            return true;
        }
        return false;
    }

    
    uint32 insertEntry(uint8 *data, BufferOffset off) {
        if (numEntries == buffSize) {
            buffSize <<= 1;
            poolData = static_cast<uint8*>(realloc(poolData, immSize * buffSize));
            if (poolData == NULL) {
                buffSize = 0;
                return -1;
            }
        }
        memcpy(&poolData[numEntries * immSize], data, immSize);
        loadOffsets.append(off.getOffset());
        return numEntries++;
    }

    bool reset() {
        numEntries = 0;
        uint32 buffSize = 8;
        poolData = static_cast<uint8*>(malloc(buffSize * immSize));
        if (poolData == NULL)
            return false;
        other = new Pool(other->maxOffset, other->immSize, other->instSize, other->bias,
                         other->alignment, other->isBackref, other->canDedup);
        if (other == NULL)
            return false;
        new (&loadOffsets) LoadOffsets;

        limitingUser = BufferOffset();
        limitingUsee = -1;
        return true;

    }
    uint32 align(uint32 ptr) {
        return (ptr + alignment-1) & ~(alignment-1);
    }
    bool isAligned(uint32 ptr) {
        return ptr == align(ptr);
    }
};

template <int SliceSize, int InstBaseSize>
struct BufferSliceTail : public BufferSlice<SliceSize> {
    Pool *data;
    uint8 isBranch[(SliceSize + (InstBaseSize * 8 - 1)) / (InstBaseSize * 8)];

    BufferSliceTail *getNext() {
        return (BufferSliceTail *)this->next;
    }
    BufferSliceTail() : data(NULL) {
        memset(isBranch, 0, sizeof(isBranch));
    }
    void markNextAsBranch() {
        int idx = this->nodeSize / InstBaseSize;
        isBranch[idx >> 3] |= 1 << (idx & 0x7);
    }
};

























template <int SliceSize, int InstBaseSize, class Inst, class Asm>
struct AssemblerBufferWithConstantPool : public AssemblerBuffer<SliceSize, Inst> {
  private:
    typedef BufferSliceTail<SliceSize, InstBaseSize> BufferSlice;
    typedef AssemblerBuffer<SliceSize, Inst> Parent;

    
    const int guardSize;
    
    const int headerSize;
    
    const int footerSize;
    
    const int numPoolKinds;

    Pool *pools;

    
    const int instBufferAlign;

    
    int numDumps;
    struct PoolInfo {
        int offset; 
        int size;   
        int finalPos; 
    };
    PoolInfo *poolInfo;
    
    
    
    int poolSize;
    
    int canNotPlacePool;
    
    bool inBackref;
    
    BufferOffset perforation;
    BufferSlice *perforatedNode;

    static const int logBasePoolInfo = 3;
    BufferSlice ** getHead() {
        return (BufferSlice**)&this->head;
    }
    BufferSlice ** getTail() {
        return (BufferSlice**)&this->tail;
    }

    virtual BufferSlice *newNode() {
        BufferSlice *tmp = static_cast<BufferSlice*>(malloc(sizeof(BufferSlice)));
        if (!tmp) {
            this->m_oom = true;
            return NULL;
        }
        new (tmp) BufferSlice;
        return tmp;
    }
  public:
    AssemblerBufferWithConstantPool(int guardSize_, int headerSize_, int footerSize_, int numPoolKinds_, Pool *pools_, int instBufferAlign_)
        : guardSize(guardSize_), headerSize(headerSize_),
          footerSize(footerSize_), numPoolKinds(numPoolKinds_),
          pools(pools_),
          instBufferAlign(instBufferAlign_), numDumps(0),
          poolInfo(static_cast<PoolInfo*>(calloc(sizeof(PoolInfo), 1 << logBasePoolInfo))),
          poolSize(0), canNotPlacePool(0), inBackref(false),
          perforatedNode(NULL)
    {
    }
    void executableCopy(uint8 *dest_) {
        if (this->oom())
            return;
        
        flushPool();
        for (int idx = 0; idx < numPoolKinds; idx++) {
            JS_ASSERT(pools[idx].numEntries == 0 && pools[idx].other->numEntries == 0);
        }
        typedef uint8 Chunk[InstBaseSize];
        Chunk *start = (Chunk*)dest_;
        Chunk *dest = (Chunk*)(((uint32)dest_ + instBufferAlign - 1) & ~(instBufferAlign -1));
        int curIndex = 0;
        int curInstOffset = 0;
        JS_ASSERT(start == dest);
        for (BufferSlice * cur = *getHead(); cur != NULL; cur = cur->getNext()) {
            Chunk *src = (Chunk*)cur->instructions;
            for (unsigned int idx = 0; idx <cur->size()/InstBaseSize;
                 idx++, curInstOffset += InstBaseSize) {
                
                if (cur->isBranch[idx >> 3] & (1<<(idx&7))) {
                    
                    patchBranch((Inst*)&src[idx], curIndex, BufferOffset(curInstOffset));
                }
                memcpy(&dest[idx], &src[idx], sizeof(Chunk));
            }
            dest+=cur->size()/InstBaseSize;
            if (cur->data != NULL) {
                
                curIndex ++;
                
                uint8 *poolDest = (uint8*)dest;
                Asm::writePoolHeader(poolDest, cur->data);
                poolDest += headerSize;
                for (int idx = 0; idx < numPoolKinds; idx++) {
                    Pool *curPool = &cur->data[idx];
                    int align = curPool->alignment-1;
                    
                    poolDest = (uint8*) (((uint32)poolDest + align) & ~align);
                    memcpy(poolDest, curPool->poolData, curPool->immSize * curPool->numEntries);
                    poolDest += curPool->immSize * curPool->numEntries;
                }
                
                for (int idx = numPoolKinds-1; idx >= 0; idx--) {
                    Pool *curPool = cur->data[idx].other;
                    int align = curPool->alignment-1;
                    
                    poolDest = (uint8*) (((uint32)poolDest + align) & ~align);
                    memcpy(poolDest, curPool->poolData, curPool->immSize * curPool->numEntries);
                    poolDest += curPool->immSize * curPool->numEntries;
                }
                
                Asm::writePoolFooter(poolDest, cur->data);
                poolDest += footerSize;
                
                dest = (Chunk*) poolDest;
            }
        }
    }

    void insertEntry(uint32 instSize, uint8 *inst, Pool *p, uint8 *data) {
        if (this->oom())
            return;
        int token;
        
        if (inBackref)
            token = insertEntryBackwards(instSize, inst, p, data);
        else
            token = insertEntryForwards(instSize, inst, p, data);
        
        if (p != NULL)
            Asm::insertTokenIntoTag(instSize, inst, token);
        
        this->putBlob(instSize, inst);
    }

    bool insertEntryBackwards(uint32 instSize, uint8 *inst, Pool *p, uint8 *data) {
        
        
        

        if (p == NULL)
            return INT_MIN;
        
        
        int codeOffset = this->bufferSize + instSize - perforation.getOffset();
        int poolOffset = footerSize;
        Pool *cur, *tmp;
        
        
        
        
        for (cur = &pools[numPoolKinds-1]; cur >= pools; cur--) {
            
            tmp = cur->other;
            if (p == tmp) {
                p->updateLimiter(this->nextOffset());
            }
            if (tmp->checkFullBackref(poolOffset, codeOffset)) {
                
                
                this->finishPool();
                return this->insertEntryForwards(instSize, inst, p, data);
            }
            poolOffset += tmp->immSize * tmp->numEntries + tmp->alignment;
            if (p == tmp) {
                poolOffset += tmp->immSize;
            }
            poolOffset += tmp->immSize * tmp->numEntries + tmp->alignment;
        }
        return p->numEntries + p->other->insertEntry(data, this->nextOffset());
    }

    
    
    
    
    
    
    int insertEntryForwards(uint32 instSize, uint8 *inst, Pool *p, uint8 *data) {
        
        uint32 nextOffset = this->bufferSize + instSize;
        uint32 poolOffset = nextOffset;
        Pool *tmp;
        
        if (true)
            poolOffset += guardSize;
        
        
        poolOffset += headerSize;

        
        for (tmp = pools; tmp < &pools[numPoolKinds]; tmp++) {
            
            JS_ASSERT((tmp->alignment & (tmp->alignment - 1)) == 0);
            poolOffset += tmp->alignment - 1;
            poolOffset &= ~(tmp->alignment - 1);
            
            
            if (p == tmp) {
                p->updateLimiter(BufferOffset(nextOffset));
            }
            if (tmp->checkFull(poolOffset)) {
                
                this->dumpPool();
                return this->insertEntryBackwards(instSize, inst, p, data);
            }
            
            if (p == tmp) {
                nextOffset += tmp->immSize;
            }
            nextOffset += tmp->immSize * tmp->numEntries;
        }
        if (p == NULL) {
            return INT_MIN;
        }
        return p->insertEntry(data, this->nextOffset());
    }
    void putInt(uint32 value) {
        insertEntry(sizeof(uint32) / sizeof(uint8), (uint8*)&value, NULL, NULL);
    }
    
    
    void perforate() {
        
        if (inBackref)
            return;
        if (canNotPlacePool)
            return;
        perforatedNode = *getTail();
        perforation = this->nextOffset();
        Parent::perforate();
    }

    
    
    
    PoolInfo getPoolData() const {
        int prevOffset = numDumps == 0 ? 0 : poolInfo[numDumps-1].offset;
        int prevEnd = numDumps == 0 ? 0 : poolInfo[numDumps-1].finalPos;
        
        int initOffset = prevEnd + (perforation.getOffset() - prevOffset);
        int finOffset = initOffset;
        bool poolIsEmpty = true;
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            if (pools[poolIdx].numEntries != 0) {
                poolIsEmpty = false;
                break;
            }
            if (pools[poolIdx].other != NULL && pools[poolIdx].other->numEntries != 0) {
                poolIsEmpty = false;
                break;
            }
        }
        if (!poolIsEmpty) {
            finOffset += headerSize;
            for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
                finOffset=pools[poolIdx].align(finOffset);
                finOffset+=pools[poolIdx].numEntries * pools[poolIdx].immSize;
            }
            
            for (int poolIdx = numPoolKinds-1; poolIdx >= 0; poolIdx--) {
                finOffset=pools[poolIdx].other->align(finOffset);
                finOffset+=pools[poolIdx].other->numEntries * pools[poolIdx].other->immSize;
            }
            finOffset += footerSize;
        }

        PoolInfo ret;
        ret.offset = perforation.getOffset();
        ret.size = finOffset - initOffset;
        ret.finalPos = finOffset;
        return ret;
    }
    void finishPool() {
        
        
        
        
        JS_ASSERT(inBackref);
        JS_ASSERT(perforatedNode != NULL);
        PoolInfo newPoolInfo = getPoolData();
        if (newPoolInfo.size == 0) {
            
            
            new (&perforation) BufferOffset();
            perforatedNode = NULL;
            inBackref = false;
            
            return;
        }
        if (numDumps >= (1<<logBasePoolInfo) && (numDumps & (numDumps-1)) == 0) {
            
            poolInfo = static_cast<PoolInfo*>(realloc(poolInfo, sizeof(PoolInfo) * numDumps * 2));
            if (poolInfo == NULL) {
                this->fail_oom();
                return;
            }
        }
        poolInfo[numDumps] = newPoolInfo;
        poolSize += poolInfo[numDumps].size;
        numDumps++;

        
        
        int poolOffset = perforation.getOffset();
        poolOffset += headerSize;
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            poolOffset=pools[poolIdx].align(poolOffset);
            poolOffset+=pools[poolIdx].numEntries * pools[poolIdx].immSize;
        }
        
        
        for (int poolIdx = numPoolKinds-1; poolIdx >= 0; poolIdx--) {
            Pool *p =  pools[poolIdx].other;
            JS_ASSERT(p != NULL);
            int idx = 0;
            
            
            
            
            
            
            
            
            
            
            

            int fakePoolOffset = poolOffset - pools[poolIdx].numEntries * pools[poolIdx].immSize;
            for (LoadOffsets::Iterator iter = p->loadOffsets.begin();
                 iter != p->loadOffsets.end(); ++iter, ++idx)
            {
                JS_ASSERT(iter->getOffset() > perforation.getOffset());
                
                Inst * inst = this->getInst(*iter);
                
                int codeOffset = fakePoolOffset - iter->getOffset();
                
                
                
                
                Asm::patchConstantPoolLoad(inst, (uint8*)inst + codeOffset);
            }
        }
        
        Pool **tmp = &perforatedNode->data;
        *tmp = static_cast<Pool*>(malloc(sizeof(Pool) * numPoolKinds));
        if (tmp == NULL) {
            this->fail_oom();
            return;
        }
        memcpy(*tmp, pools, sizeof(Pool) * numPoolKinds);

        
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            if (!pools[poolIdx].reset()) {
                this->fail_oom();
                return;
            }
        }
        new (&perforation) BufferOffset();
        perforatedNode = NULL;
        inBackref = false;
    }

    void dumpPool() {
        JS_ASSERT(!inBackref);
        if (!perforation.assigned()) {
            
            BufferOffset branch = this->nextOffset();
            this->putBlob(guardSize, NULL);
            BufferOffset afterPool = this->nextOffset();
            Asm::writePoolGuard(branch, this->getInst(branch), afterPool);
            markGuard();
        }

        
        
        int poolOffset = perforation.getOffset();
        poolOffset += headerSize;
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            Pool *p = &pools[poolIdx];
            
            poolOffset = p->align(poolOffset);
            
            
            int idx = 0;
            for (LoadOffsets::Iterator iter = p->loadOffsets.begin();
                 iter != p->loadOffsets.end(); ++iter, ++idx)
            {
                if (iter->getOffset() > perforation.getOffset()) {
                    
                    int offset = idx * p->immSize;
                    p->other->insertEntry(&p->poolData[offset], BufferOffset(*iter));
                } else {
                    
                    Inst * inst = this->getInst(*iter);
                    
                    int codeOffset = poolOffset - iter->getOffset();
                    
                    
                    
                    
                    Asm::patchConstantPoolLoad(inst, (uint8*)inst + codeOffset);
                }
            }
        }
        inBackref = true;
    }

    void flushPool() {
        if (this->oom())
            return;
        if (!inBackref)
            dumpPool();
        finishPool();
    }
    void patchBranch(Inst *i, int curpool, BufferOffset branch) {
        const Inst *ci = i;
        ptrdiff_t offset = Asm::getBranchOffset(ci);
        int destOffset = branch.getOffset() + offset;
        if (offset > 0) {
            while (poolInfo[curpool].offset <= destOffset && curpool < numDumps) {
                offset += poolInfo[curpool].size;
                curpool++;
            }
        } else {
            
            curpool--;
            while (curpool >= 0 && poolInfo[curpool].offset > destOffset) {
                offset -= poolInfo[curpool].size;
                curpool--;
            }
            
        }
        Asm::retargetBranch(i, offset);
    }

    
    void markGuard() {
        
        
        if (canNotPlacePool)
            return;
        
        
        if (inBackref)
            return;
        perforate();
    }
    void enterNoPool() {
        if (canNotPlacePool) {
            
            
        }
        canNotPlacePool++;
    }
    void leaveNoPool() {
        canNotPlacePool--;

    }
    int size() const {
        return uncheckedSize();
    }
    Pool *getPool(int idx) {
        return &pools[idx];
    }
    void markNextAsBranch() {
        JS_ASSERT(*this->getTail() != NULL);
        
        
        
        this->ensureSpace(InstBaseSize);
        (*this->getTail())->markNextAsBranch();
    }
    int uncheckedSize() const {
        PoolInfo pi = getPoolData();
        int codeEnd = this->nextOffset().getOffset();
        return (codeEnd - pi.offset) + pi.finalPos;
    }
};
} 
} 
#endif 
