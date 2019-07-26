





#ifndef jit_shared_IonAssemblerBufferWithConstantPools_h
#define jit_shared_IonAssemblerBufferWithConstantPools_h

#include "mozilla/DebugOnly.h"

#include "assembler/wtf/SegmentedVector.h"
#include "jit/IonSpewer.h"
#include "jit/shared/IonAssemblerBuffer.h"

namespace js {
namespace jit {
typedef Vector<BufferOffset, 512, OldIonAllocPolicy> LoadOffsets;

struct Pool
  : public OldIonAllocPolicy
{
    const int maxOffset;
    const int immSize;
    const int instSize;
    const int bias;

  private:
    const int alignment;

  public:
    const bool isBackref;
    const bool canDedup;
    
    Pool *other;
    uint8_t *poolData;
    uint32_t numEntries;
    uint32_t buffSize;
    LoadOffsets loadOffsets;

    
    
    
    
    
    
    
    

    BufferOffset limitingUser;
    int limitingUsee;

    Pool(int maxOffset_, int immSize_, int instSize_, int bias_, int alignment_, LifoAlloc &LifoAlloc_,
         bool isBackref_ = false, bool canDedup_ = false, Pool *other_ = nullptr)
        : maxOffset(maxOffset_), immSize(immSize_), instSize(instSize_),
          bias(bias_), alignment(alignment_),
          isBackref(isBackref_), canDedup(canDedup_), other(other_),
          poolData(static_cast<uint8_t *>(LifoAlloc_.alloc(8*immSize))), numEntries(0),
          buffSize(8), loadOffsets(), limitingUser(), limitingUsee(INT_MIN)
    {
    }
    static const int garbage=0xa5a5a5a5;
    Pool() : maxOffset(garbage), immSize(garbage), instSize(garbage), bias(garbage),
             alignment(garbage), isBackref(garbage), canDedup(garbage), other((Pool*)garbage)
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
        if (!limitingUser.assigned())
            return false;
        signed int distance =
            limitingUser.getOffset() + bias
            - codeOffset + poolOffset +
            (numEntries - limitingUsee + 1) * immSize;
        if (distance >= maxOffset)
            return true;
        return false;
    }

    
    
    

    bool checkFull(int poolOffset) {
        
        
        
        JS_ASSERT(!isBackref);
        
        if (!limitingUser.assigned()) {
            return false;
        }
        
        
        if (poolOffset + limitingUsee * immSize - (limitingUser.getOffset() + bias) >= maxOffset) {
            return true;
        }
        return false;
    }

    
    uint32_t insertEntry(uint8_t *data, BufferOffset off, LifoAlloc &LifoAlloc_) {
        if (numEntries == buffSize) {
            buffSize <<= 1;
            uint8_t *tmp = static_cast<uint8_t*>(LifoAlloc_.alloc(immSize * buffSize));
            memcpy(tmp, poolData,  immSize * numEntries);
            if (poolData == nullptr) {
                buffSize = 0;
                return -1;
            }
            poolData = tmp;
        }
        memcpy(&poolData[numEntries * immSize], data, immSize);
        loadOffsets.append(off.getOffset());
        return numEntries++;
    }

    bool reset(LifoAlloc &a) {
        numEntries = 0;
        buffSize = 8;
        poolData = static_cast<uint8_t*>(a.alloc(buffSize * immSize));
        if (poolData == nullptr)
            return false;

        void *otherSpace = a.alloc(sizeof(Pool));
        if (otherSpace == nullptr)
            return false;

        other = new (otherSpace) Pool(other->maxOffset, other->immSize, other->instSize,
                                      other->bias, other->alignment, a, other->isBackref,
                                      other->canDedup);
        new (&loadOffsets) LoadOffsets;

        limitingUser = BufferOffset();
        limitingUsee = -1;
        return true;

    }
    
    
    
    uint8_t* align(uint8_t *ptr) {
        return (uint8_t*)align((uint32_t)ptr);
    }
    uint32_t align(uint32_t ptr) {
        if (numEntries == 0)
            return ptr;
        return (ptr + alignment-1) & ~(alignment-1);
    }
    uint32_t forceAlign(uint32_t ptr) {
        return (ptr + alignment-1) & ~(alignment-1);
    }
    bool isAligned(uint32_t ptr) {
        return ptr == align(ptr);
    }
    int getAlignment() {
        return alignment;
    }

    uint32_t addPoolSize(uint32_t start) {
        start = align(start);
        start += immSize * numEntries;
        return start;
    }
    uint8_t *addPoolSize(uint8_t *start) {
        start = align(start);
        start += immSize * numEntries;
        return start;
    }
    uint32_t getPoolSize() {
        return immSize * numEntries;
    }
};


template <int SliceSize, int InstBaseSize>
struct BufferSliceTail : public BufferSlice<SliceSize> {
    Pool *data;
    mozilla::Array<uint8_t, (SliceSize + (InstBaseSize * 8 - 1)) / (InstBaseSize * 8)> isBranch;
    bool isNatural : 1;
    BufferSliceTail *getNext() {
        return (BufferSliceTail *)this->next;
    }
    BufferSliceTail() : data(nullptr), isNatural(true) {
        memset(&isBranch[0], 0, sizeof(isBranch));
    }
    void markNextAsBranch() {
        int idx = this->nodeSize / InstBaseSize;
        isBranch[idx >> 3] |= 1 << (idx & 0x7);
    }
    bool isNextBranch() {
        unsigned int size = this->nodeSize;
        if (size >= SliceSize)
            return false;
        int idx = size / InstBaseSize;
        return (isBranch[idx >> 3] >> (idx & 0x7)) & 1;
    }
};

#if 0
static int getId() {
    if (MaybeGetIonContext())
        return MaybeGetIonContext()->getNextAssemblerId();
    return NULL_ID;
}
#endif
static inline void spewEntry(uint8_t *ptr, int length) {
#if IS_LITTLE_ENDIAN
    for (int idx = 0; idx < length; idx++) {
        IonSpewCont(IonSpew_Pools, "%02x", ptr[length - idx - 1]);
        if (((idx & 3) == 3) && (idx + 1 != length))
            IonSpewCont(IonSpew_Pools, "_");
    }
#else
    for (int idx = 0; idx < length; idx++) {
        IonSpewCont(IonSpew_Pools, "%02x", ptr[idx]);
        if (((idx & 3) == 3) && (idx + 1 != length))
            IonSpewCont(IonSpew_Pools, "_");
    }
#endif
}






























template <int SliceSize, int InstBaseSize, class Inst, class Asm, int poolKindBits>
struct AssemblerBufferWithConstantPool : public AssemblerBuffer<SliceSize, Inst> {
  private:
    mozilla::Array<int, 1 << poolKindBits> entryCount;
    static const int offsetBits = 32 - poolKindBits;
  public:

    class PoolEntry {
        template <int ss, int ibs, class i, class a, int pkb>
        friend struct AssemblerBufferWithConstantPool;
        uint32_t offset_ : offsetBits;
        uint32_t kind_ : poolKindBits;
        PoolEntry(int offset, int kind) : offset_(offset), kind_(kind) {
        }
      public:
        uint32_t encode() {
            uint32_t ret;
            memcpy(&ret, this, sizeof(uint32_t));
            return ret;
        }
        PoolEntry(uint32_t bits) : offset_(((1u << offsetBits) - 1) & bits),
                                 kind_(bits >> offsetBits) {
        }
        PoolEntry() : offset_((1u << offsetBits) - 1), kind_((1u << poolKindBits) - 1) {
        }

        uint32_t poolKind() const {
            return kind_;
        }
        uint32_t offset() const {
            return offset_;
        }
    };
  private:
    typedef BufferSliceTail<SliceSize, InstBaseSize> BufferSlice;
    typedef AssemblerBuffer<SliceSize, Inst> Parent;

    
    const int guardSize;
    
    const int headerSize;
    
    const int footerSize;
    
    static const int numPoolKinds = 1 << poolKindBits;

    Pool *pools;

    
    const int instBufferAlign;

    
    int numDumps;
    struct PoolInfo {
        int offset; 
        int size;   
        int finalPos; 
        BufferSlice *slice;
    };
    PoolInfo *poolInfo;
    
    
    
    int poolSize;
    
    int canNotPlacePool;
    
    bool inBackref;
    
    BufferOffset perforation;
    BufferSlice *perforatedNode;
  public:
    int id;
  private:
    static const int logBasePoolInfo = 3;
    BufferSlice ** getHead() {
        return (BufferSlice**)&this->head;
    }
    BufferSlice ** getTail() {
        return (BufferSlice**)&this->tail;
    }

    virtual BufferSlice *newSlice(LifoAlloc &a) {
        BufferSlice *tmp = static_cast<BufferSlice*>(a.alloc(sizeof(BufferSlice)));
        if (!tmp) {
            this->m_oom = true;
            return nullptr;
        }
        new (tmp) BufferSlice;
        return tmp;
    }
  public:
    AssemblerBufferWithConstantPool(int guardSize_, int headerSize_, int footerSize_, Pool *pools_, int instBufferAlign_)
        : guardSize(guardSize_), headerSize(headerSize_),
          footerSize(footerSize_),
          pools(pools_),
          instBufferAlign(instBufferAlign_), numDumps(0),
          poolInfo(nullptr),
          poolSize(0), canNotPlacePool(0), inBackref(false),
          perforatedNode(nullptr), id(-1)
    {
        for (int idx = 0; idx < numPoolKinds; idx++) {
            entryCount[idx] = 0;
        }
    }

    
    
    void initWithAllocator() {
        poolInfo = static_cast<PoolInfo*>(this->LifoAlloc_.alloc(sizeof(PoolInfo) * (1 << logBasePoolInfo)));
    }

    const PoolInfo & getInfo(int x) const {
        static const PoolInfo nil = {0,0,0};
        if (x < 0 || x >= numDumps)
            return nil;
        return poolInfo[x];
    }
    void executableCopy(uint8_t *dest_) {
        if (this->oom())
            return;
        
        flushPool();
        for (int idx = 0; idx < numPoolKinds; idx++) {
            JS_ASSERT(pools[idx].numEntries == 0 && pools[idx].other->numEntries == 0);
        }
        typedef mozilla::Array<uint8_t, InstBaseSize> Chunk;
        mozilla::DebugOnly<Chunk *> start = (Chunk*)dest_;
        Chunk *dest = (Chunk*)(((uint32_t)dest_ + instBufferAlign - 1) & ~(instBufferAlign -1));
        int curIndex = 0;
        int curInstOffset = 0;
        JS_ASSERT(start == dest);
        for (BufferSlice * cur = *getHead(); cur != nullptr; cur = cur->getNext()) {
            Chunk *src = (Chunk*)&cur->instructions;
            for (unsigned int idx = 0; idx <cur->size()/InstBaseSize;
                 idx++, curInstOffset += InstBaseSize) {
                
                if (cur->isBranch[idx >> 3] & (1<<(idx&7))) {
                    
                    patchBranch((Inst*)&src[idx], curIndex, BufferOffset(curInstOffset));
                }
                memcpy(&dest[idx], &src[idx], sizeof(Chunk));
            }
            dest+=cur->size()/InstBaseSize;
            if (cur->data != nullptr) {
                
                curIndex ++;
                
                uint8_t *poolDest = (uint8_t*)dest;
                Asm::writePoolHeader(poolDest, cur->data, cur->isNatural);
                poolDest += headerSize;
                for (int idx = 0; idx < numPoolKinds; idx++) {
                    Pool *curPool = &cur->data[idx];
                    
                    poolDest = curPool->align(poolDest);
                    memcpy(poolDest, curPool->poolData, curPool->immSize * curPool->numEntries);
                    poolDest += curPool->immSize * curPool->numEntries;
                }
                
                for (int idx = numPoolKinds-1; idx >= 0; idx--) {
                    Pool *curPool = cur->data[idx].other;
                    
                    poolDest = curPool->align(poolDest);
                    memcpy(poolDest, curPool->poolData, curPool->immSize * curPool->numEntries);
                    poolDest += curPool->immSize * curPool->numEntries;
                }
                
                Asm::writePoolFooter(poolDest, cur->data, cur->isNatural);
                poolDest += footerSize;
                
                dest = (Chunk*) poolDest;
            }
        }
    }

    BufferOffset insertEntry(uint32_t instSize, uint8_t *inst, Pool *p, uint8_t *data, PoolEntry *pe = nullptr) {
        if (this->oom() && !this->bail())
            return BufferOffset();
        int token;
        if (p != nullptr) {
            int poolId = p - pools;
            const char sigil = inBackref ? 'B' : 'F';

            IonSpew(IonSpew_Pools, "[%d]{%c} Inserting entry into pool %d", id, sigil, poolId);
            IonSpewStart(IonSpew_Pools, "[%d] data is: 0x", id);
            spewEntry(data, p->immSize);
            IonSpewFin(IonSpew_Pools);
        }
        
        if (inBackref)
            token = insertEntryBackwards(instSize, inst, p, data);
        else
            token = insertEntryForwards(instSize, inst, p, data);
        
        PoolEntry retPE;
        if (p != nullptr) {
            if (this->oom())
                return BufferOffset();
            int poolId = p - pools;
            IonSpew(IonSpew_Pools, "[%d] Entry has token %d, offset ~%d", id, token, size());
            Asm::insertTokenIntoTag(instSize, inst, token);
            JS_ASSERT(poolId < (1 << poolKindBits));
            JS_ASSERT(poolId >= 0);
            
            retPE = PoolEntry(entryCount[poolId], poolId);
            entryCount[poolId]++;
        }
        
        if (pe != nullptr)
            *pe = retPE;
        return this->putBlob(instSize, inst);
    }

    uint32_t insertEntryBackwards(uint32_t instSize, uint8_t *inst, Pool *p, uint8_t *data) {
        
        
        

        if (p == nullptr)
            return INT_MIN;
        
        
        int poolOffset = footerSize;
        Pool *cur, *tmp;
        
        
        
        
        for (cur = pools; cur < &pools[numPoolKinds]; cur++) {
            
            tmp = cur->other;
            if (p == cur)
                tmp->updateLimiter(this->nextOffset());

            if (tmp->checkFullBackref(poolOffset, perforation.getOffset())) {
                
                
                if (p != nullptr)
                    IonSpew(IonSpew_Pools, "[%d]Inserting pool entry caused a spill", id);
                else
                    IonSpew(IonSpew_Pools, "[%d]Inserting instruction(%d) caused a spill", id, size());

                this->finishPool();
                if (this->oom())
                    return uint32_t(-1);
                return this->insertEntryForwards(instSize, inst, p, data);
            }
            
            
            poolOffset += tmp->immSize * tmp->numEntries + tmp->getAlignment();
            if (p == tmp) {
                poolOffset += tmp->immSize;
            }
        }
        return p->numEntries + p->other->insertEntry(data, this->nextOffset(), this->LifoAlloc_);
    }

    
    
    
    
    
    
    int insertEntryForwards(uint32_t instSize, uint8_t *inst, Pool *p, uint8_t *data) {
        
        uint32_t nextOffset = this->size() + instSize;
        uint32_t poolOffset = nextOffset;
        Pool *tmp;
        
        if (!perforatedNode)
            poolOffset += guardSize;
        
        
        poolOffset += headerSize;

        
        for (tmp = pools; tmp < &pools[numPoolKinds]; tmp++) {
            
            JS_ASSERT((tmp->getAlignment() & (tmp->getAlignment() - 1)) == 0);
            
            
            
            if (p == tmp)
                poolOffset = tmp->forceAlign(poolOffset);
            else
                poolOffset = tmp->align(poolOffset);

            
            
            if (p == tmp) {
                p->updateLimiter(BufferOffset(nextOffset));
            }
            if (tmp->checkFull(poolOffset)) {
                
                if (p != nullptr)
                    IonSpew(IonSpew_Pools, "[%d] Inserting pool entry caused a spill", id);
                else
                    IonSpew(IonSpew_Pools, "[%d] Inserting instruction(%d) caused a spill", id, size());

                this->dumpPool();
                return this->insertEntryBackwards(instSize, inst, p, data);
            }
            
            if (p == tmp) {
                nextOffset += tmp->immSize;
            }
            nextOffset += tmp->immSize * tmp->numEntries;
        }
        if (p == nullptr) {
            return INT_MIN;
        }
        return p->insertEntry(data, this->nextOffset(), this->LifoAlloc_);
    }
    BufferOffset putInt(uint32_t value) {
        return insertEntry(sizeof(uint32_t) / sizeof(uint8_t), (uint8_t*)&value, nullptr, nullptr);
    }
    
    
    void perforate() {
        
        if (inBackref)
            return;
        if (canNotPlacePool)
            return;
        
        
        bool empty = true;
        for (int i = 0; i < numPoolKinds; i++) {
            if (pools[i].numEntries != 0) {
                empty = false;
                break;
            }
        }
        if (empty)
            return;
        perforatedNode = *getTail();
        perforation = this->nextOffset();
        Parent::perforate();
        IonSpew(IonSpew_Pools, "[%d] Adding a perforation at offset %d", id, perforation.getOffset());
    }

    
    
    
    PoolInfo getPoolData() const {
        int prevOffset = getInfo(numDumps-1).offset;
        int prevEnd = getInfo(numDumps-1).finalPos;
        
        int perfOffset = perforation.assigned() ?
            perforation.getOffset() :
            this->nextOffset().getOffset() + this->guardSize;
        int initOffset = prevEnd + (perfOffset - prevOffset);
        int finOffset = initOffset;
        bool poolIsEmpty = true;
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            if (pools[poolIdx].numEntries != 0) {
                poolIsEmpty = false;
                break;
            }
            if (pools[poolIdx].other != nullptr && pools[poolIdx].other->numEntries != 0) {
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
        ret.offset = perfOffset;
        ret.size = finOffset - initOffset;
        ret.finalPos = finOffset;
        ret.slice = perforatedNode;
        return ret;
    }
    void finishPool() {
        
        
        
        
        IonSpew(IonSpew_Pools, "[%d] Finishing pool %d", id, numDumps);
        JS_ASSERT(inBackref);
        PoolInfo newPoolInfo = getPoolData();
        if (newPoolInfo.size == 0) {
            
            
            new (&perforation) BufferOffset();
            perforatedNode = nullptr;
            inBackref = false;
            IonSpew(IonSpew_Pools, "[%d] Aborting because the pool is empty", id);
            
            return;
        }
        JS_ASSERT(perforatedNode != nullptr);
        if (numDumps >= (1<<logBasePoolInfo) && (numDumps & (numDumps-1)) == 0) {
            
            PoolInfo *tmp = static_cast<PoolInfo*>(this->LifoAlloc_.alloc(sizeof(PoolInfo) * numDumps * 2));
            if (tmp == nullptr) {
                this->fail_oom();
                return;
            }
            memcpy(tmp, poolInfo, sizeof(PoolInfo) * numDumps);
            poolInfo = tmp;

        }

        
        
        int poolOffset = perforation.getOffset();
        int magicAlign = getInfo(numDumps-1).finalPos - getInfo(numDumps-1).offset;
        poolOffset += magicAlign;
        poolOffset += headerSize;
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            poolOffset=pools[poolIdx].align(poolOffset);
            poolOffset+=pools[poolIdx].numEntries * pools[poolIdx].immSize;
        }
        mozilla::Array<LoadOffsets, 1 << poolKindBits> outcasts;
        mozilla::Array<uint8_t *, 1 << poolKindBits> outcastEntries;
        
        
        int skippedBytes = 0;
        for (int poolIdx = numPoolKinds-1; poolIdx >= 0; poolIdx--) {
            Pool *p =  pools[poolIdx].other;
            JS_ASSERT(p != nullptr);
            unsigned int idx = p->numEntries-1;
            
            
            
            outcastEntries[poolIdx] = new uint8_t[p->getPoolSize()];
            bool *preservedEntries = new bool[p->numEntries];
            
            
            
            
            
            
            
            
            
            
            
            poolOffset = p->align(poolOffset);
            int numSkips = 0;
            int fakePoolOffset = poolOffset - pools[poolIdx].numEntries * pools[poolIdx].immSize;
            for (BufferOffset *iter = p->loadOffsets.end()-1;
                 iter != p->loadOffsets.begin()-1; --iter, --idx)
            {

                IonSpew(IonSpew_Pools, "[%d] Linking entry %d in pool %d", id, idx+ pools[poolIdx].numEntries, poolIdx);
                JS_ASSERT(iter->getOffset() >= perforation.getOffset());
                
                Inst * inst = this->getInst(*iter);
                
                
                int codeOffset = fakePoolOffset - iter->getOffset() - newPoolInfo.size + numSkips * p->immSize - skippedBytes;
                
                
                
                
                IonSpew(IonSpew_Pools, "[%d] Fixing offset to %d", id, codeOffset - magicAlign);
                if (!Asm::patchConstantPoolLoad(inst, (uint8_t*)inst + codeOffset - magicAlign)) {
                    
                    
                    
                    
                    
                    
                    IonSpew(IonSpew_Pools, "[%d]***Offset was still out of range!***", id, codeOffset - magicAlign);
                    IonSpew(IonSpew_Pools, "[%d] Too complicated; bailingp", id);
                    this->fail_bail();
                    
                    for (int pi = poolIdx; pi < numPoolKinds; pi++)
                        delete[] outcastEntries[pi];
                    delete[] preservedEntries;
                    return;
                } else {
                    preservedEntries[idx] = true;
                }
            }
            
            unsigned int idxDest = 0;
            
            if (numSkips != 0) {
                for (idx = 0; idx < p->numEntries; idx++) {
                    if (preservedEntries[idx]) {
                        if (idx != idxDest) {
                            memcpy(&p->poolData[idxDest * p->immSize],
                                   &p->poolData[idx * p->immSize],
                                   p->immSize);
                        }
                        idxDest++;
                    }
                }
                p->numEntries -= numSkips;
            }
            poolOffset += p->numEntries * p->immSize;
            delete[] preservedEntries;
            preservedEntries = nullptr;
        }
        
        Pool **tmp = &perforatedNode->data;
        *tmp = static_cast<Pool*>(this->LifoAlloc_.alloc(sizeof(Pool) * numPoolKinds));
        if (tmp == nullptr) {
            this->fail_oom();
            for (int pi = 0; pi < numPoolKinds; pi++)
                delete[] outcastEntries[pi];
            return;
        }
        
        
        newPoolInfo = getPoolData();
        poolInfo[numDumps] = newPoolInfo;
        poolSize += poolInfo[numDumps].size;
        numDumps++;

        memcpy(*tmp, pools, sizeof(Pool) * numPoolKinds);

        
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            if (!pools[poolIdx].reset(this->LifoAlloc_)) {
                this->fail_oom();
                for (int pi = 0; pi < numPoolKinds; pi++)
                    delete[] outcastEntries[pi];
                return;
            }
        }
        new (&perforation) BufferOffset();
        perforatedNode = nullptr;
        inBackref = false;

        
        
        
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            
            
            
            
            int idx = outcasts[poolIdx].length();
            for (BufferOffset *iter = outcasts[poolIdx].end()-1;
                 iter != outcasts[poolIdx].begin()-1;
                 --iter, --idx) {
                pools[poolIdx].updateLimiter(*iter);
                Inst *inst = this->getInst(*iter);
                Asm::insertTokenIntoTag(pools[poolIdx].instSize, (uint8_t*)inst, outcasts[poolIdx].end()-1-iter);
                pools[poolIdx].insertEntry(&outcastEntries[poolIdx][idx*pools[poolIdx].immSize], *iter, this->LifoAlloc_);
            }
            delete[] outcastEntries[poolIdx];
        }
        
        
        
        
        poolOffset = this->size() + guardSize * 2;
        poolOffset += headerSize;
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            
            
            
            
            poolOffset = pools[poolIdx].align(poolOffset);
            if (pools[poolIdx].checkFull(poolOffset)) {
                
                dumpPool();
                break;
            }
            poolOffset += pools[poolIdx].getPoolSize();
        }
    }

    void dumpPool() {
        JS_ASSERT(!inBackref);
        IonSpew(IonSpew_Pools, "[%d] Attempting to dump the pool", id);
        PoolInfo newPoolInfo = getPoolData();
        if (newPoolInfo.size == 0) {
            
            inBackref = true;
            IonSpew(IonSpew_Pools, "[%d]Abort, no pool data", id);
            return;
        }

        IonSpew(IonSpew_Pools, "[%d] Dumping %d bytes", id, newPoolInfo.size);
        if (!perforation.assigned()) {
            IonSpew(IonSpew_Pools, "[%d] No Perforation point selected, generating a new one", id);
            
            BufferOffset branch = this->nextOffset();
            bool shouldMarkAsBranch = this->isNextBranch();
            this->markNextAsBranch();
            this->putBlob(guardSize, nullptr);
            BufferOffset afterPool = this->nextOffset();
            Asm::writePoolGuard(branch, this->getInst(branch), afterPool);
            markGuard();
            perforatedNode->isNatural = false;
            if (shouldMarkAsBranch)
                this->markNextAsBranch();
        }

        
        
        int poolOffset = perforation.getOffset();
        int magicAlign =  getInfo(numDumps-1).finalPos - getInfo(numDumps-1).offset;
        poolOffset += magicAlign;
        poolOffset += headerSize;
        for (int poolIdx = 0; poolIdx < numPoolKinds; poolIdx++) {
            mozilla::DebugOnly<bool> beforePool = true;
            Pool *p = &pools[poolIdx];
            
            
            int idx = 0;
            for (BufferOffset *iter = p->loadOffsets.begin();
                 iter != p->loadOffsets.end(); ++iter, ++idx)
            {
                if (iter->getOffset() >= perforation.getOffset()) {
                    IonSpew(IonSpew_Pools, "[%d] Pushing entry %d in pool %d into the backwards section.", id, idx, poolIdx);
                    
                    int offset = idx * p->immSize;
                    p->other->insertEntry(&p->poolData[offset], BufferOffset(*iter), this->LifoAlloc_);
                    
                    p->other->updateLimiter(*iter);

                    
                    
                    p->numEntries--;
                    beforePool = false;
                } else {
                    JS_ASSERT(beforePool);
                    
                    
                    
                    poolOffset = p->align(poolOffset);
                    IonSpew(IonSpew_Pools, "[%d] Entry %d in pool %d is before the pool.", id, idx, poolIdx);
                    
                    Inst * inst = this->getInst(*iter);
                    
                    int codeOffset = poolOffset - iter->getOffset();
                    
                    
                    
                    
                    IonSpew(IonSpew_Pools, "[%d] Fixing offset to %d", id, codeOffset - magicAlign);
                    Asm::patchConstantPoolLoad(inst, (uint8_t*)inst + codeOffset - magicAlign);
                }
            }
            
            
            
            poolOffset += p->numEntries * p->immSize;
        }
        poolOffset = footerSize;
        inBackref = true;
        for (int poolIdx = numPoolKinds-1; poolIdx >= 0; poolIdx--) {
            Pool *tmp = pools[poolIdx].other;
            if (tmp->checkFullBackref(poolOffset, perforation.getOffset())) {
                
                
                finishPool();
                break;
            }
        }
    }

    void flushPool() {
        if (this->oom())
            return;
        IonSpew(IonSpew_Pools, "[%d] Requesting a pool flush", id);
        if (!inBackref)
            dumpPool();
        finishPool();
    }
    void patchBranch(Inst *i, int curpool, BufferOffset branch) {
        const Inst *ci = i;
        ptrdiff_t offset = Asm::getBranchOffset(ci);
        
        if (offset == 0)
            return;
        int destOffset = branch.getOffset() + offset;
        if (offset > 0) {

            while (curpool < numDumps && poolInfo[curpool].offset <= destOffset) {
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
        Asm::retargetNearBranch(i, offset, false);
    }

    
    void markGuard() {
        
        
        if (canNotPlacePool)
            return;
        
        
        if (inBackref)
            return;
        perforate();
    }
    void enterNoPool() {
        if (!canNotPlacePool && !perforation.assigned()) {
            
            
            
            
            
            
            

            
            
            
            

            
            

            
            
            
            
            

            BufferOffset branch = this->nextOffset();
            this->markNextAsBranch();
            this->putBlob(guardSize, nullptr);
            BufferOffset afterPool = this->nextOffset();
            Asm::writePoolGuard(branch, this->getInst(branch), afterPool);
            markGuard();
            if (perforatedNode != nullptr)
                perforatedNode->isNatural = false;
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
        
        
        
        this->ensureSpace(InstBaseSize);
        JS_ASSERT(*this->getTail() != nullptr);
        (*this->getTail())->markNextAsBranch();
    }
    bool isNextBranch() {
        JS_ASSERT(*this->getTail() != nullptr);
        return (*this->getTail())->isNextBranch();
    }

    int uncheckedSize() const {
        PoolInfo pi = getPoolData();
        int codeEnd = this->nextOffset().getOffset();
        return (codeEnd - pi.offset) + pi.finalPos;
    }
    ptrdiff_t curDumpsite;
    void resetCounter() {
        curDumpsite = 0;
    }
    ptrdiff_t poolSizeBefore(ptrdiff_t offset) const {
        int cur = 0;
        while(cur < numDumps && poolInfo[cur].offset <= offset)
            cur++;
        
        
        if (cur == 0)
            return 0;
        return poolInfo[cur-1].finalPos - poolInfo[cur-1].offset;
    }

  private:
    void getPEPool(PoolEntry pe, Pool **retP, int32_t * retOffset, int32_t *poolNum) const {
        int poolKind = pe.poolKind();
        Pool *p = nullptr;
        uint32_t offset = pe.offset() * pools[poolKind].immSize;
        int idx;
        for (idx = 0; idx < numDumps; idx++) {
            p = &poolInfo[idx].slice->data[poolKind];
            if (p->getPoolSize() > offset)
                break;
            offset -= p->getPoolSize();
            p = p->other;
            if (p->getPoolSize() > offset)
                break;
            offset -= p->getPoolSize();
            p = nullptr;
        }
        if (poolNum != nullptr)
            *poolNum = idx;
        
        
        
        if (p == nullptr) {
            p = &pools[poolKind];
            if (offset >= p->getPoolSize()) {
                p = p->other;
                offset -= p->getPoolSize();
            }
        }
        JS_ASSERT(p != nullptr);
        JS_ASSERT(offset < p->getPoolSize());
        *retP = p;
        *retOffset = offset;
    }
    uint8_t *getPoolEntry(PoolEntry pe) {
        Pool *p;
        int32_t offset;
        getPEPool(pe, &p, &offset, nullptr);
        return &p->poolData[offset];
    }
    size_t getPoolEntrySize(PoolEntry pe) {
        int idx = pe.poolKind();
        return pools[idx].immSize;
    }

  public:
    uint32_t poolEntryOffset(PoolEntry pe) const {
        Pool *realPool;
        
        int32_t offset;
        int32_t poolNum;
        getPEPool(pe, &realPool, &offset, &poolNum);
        PoolInfo *pi = &poolInfo[poolNum];
        Pool *poolGroup = pi->slice->data;
        uint32_t start = pi->finalPos - pi->size + headerSize;
        
        
        
        
        for (int idx = 0; idx < numPoolKinds; idx++) {
            if (&poolGroup[idx] == realPool) {
                return start + offset;
            }
            start = poolGroup[idx].addPoolSize(start);
        }
        for (int idx = numPoolKinds-1; idx >= 0; idx--) {
            if (poolGroup[idx].other == realPool) {
                return start + offset;
            }
            start = poolGroup[idx].other->addPoolSize(start);
        }
        MOZ_ASSUME_UNREACHABLE("Entry is not in a pool");
    }
    void writePoolEntry(PoolEntry pe, uint8_t *buff) {
        size_t size = getPoolEntrySize(pe);
        uint8_t *entry = getPoolEntry(pe);
        memcpy(entry, buff, size);
    }
    void readPoolEntry(PoolEntry pe, uint8_t *buff) {
        size_t size = getPoolEntrySize(pe);
        uint8_t *entry = getPoolEntry(pe);
        memcpy(buff, entry, size);
    }

};
} 
} 
#endif 
