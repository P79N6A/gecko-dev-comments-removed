





#ifndef jit_shared_IonAssemblerBufferWithConstantPools_h
#define jit_shared_IonAssemblerBufferWithConstantPools_h

#include "mozilla/DebugOnly.h"

#include "jit/JitSpewer.h"
#include "jit/shared/IonAssemblerBuffer.h"
































































































namespace js {
namespace jit {

typedef Vector<BufferOffset, 512, OldIonAllocPolicy> LoadOffsets;


typedef int32_t PoolAllocUnit;

struct Pool
  : public OldIonAllocPolicy
{
  private:
    
    
    
    
    const size_t maxOffset_;
    
    
    const unsigned bias_;

    
    unsigned numEntries_;
    
    unsigned buffSize;
    
    PoolAllocUnit *poolData_;

    
    
    
    
    
    
    
    
    BufferOffset limitingUser;
    
    unsigned limitingUsee;

  public:
    
    
    
    
    LoadOffsets loadOffsets;

    explicit Pool(size_t maxOffset, unsigned bias, LifoAlloc &lifoAlloc)
        : maxOffset_(maxOffset), bias_(bias), numEntries_(0), buffSize(8),
          poolData_(lifoAlloc.newArrayUninitialized<PoolAllocUnit>(buffSize)),
          limitingUser(), limitingUsee(INT_MIN), loadOffsets()
    {
    }
    static const unsigned Garbage = 0xa5a5a5a5;
    Pool() : maxOffset_(Garbage), bias_(Garbage)
    {
    }

    PoolAllocUnit *poolData() const {
        return poolData_;
    }

    unsigned numEntries() const {
        return numEntries_;
    }

    size_t getPoolSize() const {
        return numEntries_ * sizeof(PoolAllocUnit);
    }

    
    
    
    
    
    
    
    
    void updateLimiter(BufferOffset nextInst) {
        ptrdiff_t oldRange = limitingUsee * sizeof(PoolAllocUnit) - limitingUser.getOffset();
        ptrdiff_t newRange = numEntries_ * sizeof(PoolAllocUnit) - nextInst.getOffset();
        if (!limitingUser.assigned() || newRange > oldRange) {
            
            limitingUser = nextInst;
            limitingUsee = numEntries_;
        }
    }

    
    
    
    
    
    bool checkFull(size_t poolOffset) const {
        
        if (!limitingUser.assigned())
            return false;
        size_t offset = poolOffset + limitingUsee * sizeof(PoolAllocUnit)
                        - (limitingUser.getOffset() + bias_);
        return offset >= maxOffset_;
    }

    unsigned insertEntry(unsigned num, uint8_t *data, BufferOffset off, LifoAlloc &lifoAlloc) {
        if (numEntries_ + num >= buffSize) {
            
            buffSize *= 2;
            PoolAllocUnit *tmp = lifoAlloc.newArrayUninitialized<PoolAllocUnit>(buffSize);
            if (poolData_ == nullptr) {
                buffSize = 0;
                return -1;
            }
            mozilla::PodCopy(tmp, poolData_, numEntries_);
            poolData_ = tmp;
        }
        mozilla::PodCopy(&poolData_[numEntries_], (PoolAllocUnit *)data, num);
        loadOffsets.append(off.getOffset());
        unsigned ret = numEntries_;
        numEntries_ += num;
        return ret;
    }

    bool reset(LifoAlloc &a) {
        numEntries_ = 0;
        buffSize = 8;
        poolData_ = static_cast<PoolAllocUnit *>(a.alloc(buffSize * sizeof(PoolAllocUnit)));
        if (poolData_ == nullptr)
            return false;

        new (&loadOffsets) LoadOffsets;

        limitingUser = BufferOffset();
        limitingUsee = -1;
        return true;
    }

};


template <size_t SliceSize, size_t InstSize>
struct BufferSliceTail : public BufferSlice<SliceSize> {
  private:
    
    
    mozilla::Array<uint8_t, (SliceSize / InstSize) / 8> isBranch_;
  public:
    Pool *pool;
    
    
    
    
    bool isNatural : 1;
    BufferSliceTail *getNext() const {
        return (BufferSliceTail *)this->next_;
    }
    explicit BufferSliceTail() : pool(nullptr), isNatural(true) {
        static_assert(SliceSize % (8 * InstSize) == 0, "SliceSize must be a multple of 8 * InstSize.");
        mozilla::PodArrayZero(isBranch_);
    }
    void markNextAsBranch() {
        
        
        
        MOZ_ASSERT(this->nodeSize_ % InstSize == 0);
        MOZ_ASSERT(this->nodeSize_ < SliceSize);
        size_t idx = this->nodeSize_ / InstSize;
        isBranch_[idx >> 3] |= 1 << (idx & 0x7);
    }
    bool isBranch(unsigned idx) const {
        MOZ_ASSERT(idx < this->nodeSize_ / InstSize);
        return (isBranch_[idx >> 3] >> (idx & 0x7)) & 1;
    }
    bool isNextBranch() const {
        size_t size = this->nodeSize_;
        MOZ_ASSERT(size < SliceSize);
        return isBranch(size / InstSize);
    }
};




template <size_t SliceSize, size_t InstSize, class Inst, class Asm>
struct AssemblerBufferWithConstantPools : public AssemblerBuffer<SliceSize, Inst> {
  private:
    
    
    
    size_t poolEntryCount;
  public:
    class PoolEntry {
        size_t index_;
      public:
        explicit PoolEntry(size_t index) : index_(index) {
        }
        PoolEntry() : index_(-1) {
        }
        size_t index() const {
            return index_;
        }
    };

  private:
    typedef BufferSliceTail<SliceSize, InstSize> BufferSlice;
    typedef AssemblerBuffer<SliceSize, Inst> Parent;

    
    const unsigned guardSize_;
    
    
    const unsigned headerSize_;

    
    
    
    
    const size_t poolMaxOffset_;

    
    
    const unsigned pcBias_;

    
    Pool pool_;

    
    const size_t instBufferAlign_;

    struct PoolInfo {
        
        
        size_t offset;
        
        unsigned size;
        
        
        
        
        
        
        size_t finalPos;
        
        BufferSlice *slice;
    };
    
    
    unsigned numDumps_;
    
    size_t poolInfoSize_;
    
    PoolInfo *poolInfo_;

    
    bool canNotPlacePool_;

#ifdef DEBUG
    
    
    size_t canNotPlacePoolStartOffset_;
    
    
    size_t canNotPlacePoolMaxInst_;
#endif

    
    const uint32_t alignFillInst_;

    
    
    
    
    const uint32_t nopFillInst_;
    const unsigned nopFill_;
    
    
    bool inhibitNops_;

  public:

    
    
    int id;

  private:
    
    
    BufferSlice *getHead() const {
        return (BufferSlice *)this->head;
    }
    BufferSlice *getTail() const {
        return (BufferSlice *)this->tail;
    }

    virtual BufferSlice *newSlice(LifoAlloc &a) {
        BufferSlice *slice = a.new_<BufferSlice>();
        if (!slice) {
            this->m_oom = true;
            return nullptr;
        }
        return slice;
    }

  public:
    AssemblerBufferWithConstantPools(unsigned guardSize, unsigned headerSize,
                                     size_t instBufferAlign, size_t poolMaxOffset,
                                     unsigned pcBias, uint32_t alignFillInst, uint32_t nopFillInst,
                                     unsigned nopFill = 0)
        : poolEntryCount(0), guardSize_(guardSize), headerSize_(headerSize),
          poolMaxOffset_(poolMaxOffset), pcBias_(pcBias),
          instBufferAlign_(instBufferAlign),
          numDumps_(0), poolInfoSize_(8), poolInfo_(nullptr),
          canNotPlacePool_(false), alignFillInst_(alignFillInst),
          nopFillInst_(nopFillInst), nopFill_(nopFill), inhibitNops_(false),
          id(-1)
    {
    }

    
    
    void initWithAllocator() {
        poolInfo_ = this->lifoAlloc_.template newArrayUninitialized<PoolInfo>(poolInfoSize_);

        new (&pool_) Pool (poolMaxOffset_, pcBias_, this->lifoAlloc_);
        if (pool_.poolData() == nullptr)
            this->fail_oom();
    }

  private:
    const PoolInfo &getLastPoolInfo() const {
        
        
        static const PoolInfo nil = {0, 0, 0, nullptr};

        if (numDumps_ > 0)
            return poolInfo_[numDumps_ - 1];

        return nil;
    }

    size_t sizeExcludingCurrentPool() const {
        
        
        size_t codeEnd = this->nextOffset().getOffset();
        
        
        PoolInfo pi = getLastPoolInfo();
        return codeEnd + (pi.finalPos - pi.offset);
    }

  public:
    size_t size() const {
        
        
        MOZ_ASSERT(pool_.numEntries() == 0);
        return sizeExcludingCurrentPool();
    }

  private:
    void insertNopFill() {
        
        if (nopFill_ > 0 && !inhibitNops_ && !canNotPlacePool_) {
            inhibitNops_ = true;

            
            
            for (size_t i = 0; i < nopFill_; i++)
                putInt(nopFillInst_);

            inhibitNops_ = false;
        }
    }

    void markNextAsBranch() {
        
        
        this->ensureSpace(InstSize);
        MOZ_ASSERT(this->getTail() != nullptr);
        this->getTail()->markNextAsBranch();
    }

    bool isNextBranch() const {
        MOZ_ASSERT(this->getTail() != nullptr);
        return this->getTail()->isNextBranch();
    }

    int insertEntryForwards(unsigned numInst, unsigned numPoolEntries, uint8_t *inst, uint8_t *data) {
        size_t nextOffset = sizeExcludingCurrentPool();
        size_t poolOffset = nextOffset + (numInst + guardSize_ + headerSize_) * InstSize;

        

        
        
        if (numPoolEntries)
            pool_.updateLimiter(BufferOffset(nextOffset));

        if (pool_.checkFull(poolOffset)) {
            if (numPoolEntries)
                JitSpew(JitSpew_Pools, "[%d] Inserting pool entry caused a spill", id);
            else
                JitSpew(JitSpew_Pools, "[%d] Inserting instruction(%d) caused a spill", id,
                        sizeExcludingCurrentPool());

            finishPool();
            if (this->oom())
                return uint32_t(-1);
            return insertEntryForwards(numInst, numPoolEntries, inst, data);
        }
        if (numPoolEntries)
            return pool_.insertEntry(numPoolEntries, data, this->nextOffset(), this->lifoAlloc_);

        
        
        
        return UINT_MAX;
    }

  public:
    BufferOffset allocEntry(size_t numInst, unsigned numPoolEntries,
                            uint8_t *inst, uint8_t *data, PoolEntry *pe = nullptr,
                            bool markAsBranch = false) {
        
        
        MOZ_ASSERT_IF(numPoolEntries, !canNotPlacePool_);

        if (this->oom() && !this->bail())
            return BufferOffset();

        insertNopFill();

#ifdef DEBUG
        if (numPoolEntries) {
            JitSpew(JitSpew_Pools, "[%d] Inserting %d entries into pool", id, numPoolEntries);
            JitSpewStart(JitSpew_Pools, "[%d] data is: 0x", id);
            size_t length = numPoolEntries * sizeof(PoolAllocUnit);
            for (unsigned idx = 0; idx < length; idx++) {
                JitSpewCont(JitSpew_Pools, "%02x", data[length - idx - 1]);
                if (((idx & 3) == 3) && (idx + 1 != length))
                    JitSpewCont(JitSpew_Pools, "_");
            }
            JitSpewFin(JitSpew_Pools);
        }
#endif

        
        unsigned index = insertEntryForwards(numInst, numPoolEntries, inst, data);
        
        PoolEntry retPE;
        if (numPoolEntries) {
            if (this->oom())
                return BufferOffset();
            JitSpew(JitSpew_Pools, "[%d] Entry has index %u, offset %u", id, index,
                    sizeExcludingCurrentPool());
            Asm::InsertIndexIntoTag(inst, index);
            
            retPE = PoolEntry(poolEntryCount);
            poolEntryCount += numPoolEntries;
        }
        
        if (pe != nullptr)
            *pe = retPE;
        if (markAsBranch)
            markNextAsBranch();
        return this->putBlob(numInst * InstSize, inst);
    }

    BufferOffset putInt(uint32_t value, bool markAsBranch = false) {
        return allocEntry(1, 0, (uint8_t *)&value, nullptr, nullptr, markAsBranch);
    }

  private:
    PoolInfo getPoolData(BufferSlice *perforatedSlice, size_t perfOffset) const {
        PoolInfo pi = getLastPoolInfo();
        size_t prevOffset = pi.offset;
        size_t prevEnd = pi.finalPos;
        
        size_t initOffset = perfOffset + (prevEnd - prevOffset);
        size_t finOffset = initOffset;
        if (pool_.numEntries() != 0) {
            finOffset += headerSize_ * InstSize;
            finOffset += pool_.getPoolSize();
        }

        PoolInfo ret;
        ret.offset = perfOffset;
        ret.size = finOffset - initOffset;
        ret.finalPos = finOffset;
        ret.slice = perforatedSlice;
        return ret;
    }

    void finishPool() {
        JitSpew(JitSpew_Pools, "[%d] Attempting to finish pool %d with %d entries.", id,
                numDumps_, pool_.numEntries());

        if (pool_.numEntries() == 0) {
            
            JitSpew(JitSpew_Pools, "[%d] Aborting because the pool is empty", id);
            return;
        }

        
        MOZ_ASSERT(!canNotPlacePool_);

        
        BufferOffset branch = this->nextOffset();
        
        markNextAsBranch();
        this->putBlob(guardSize_ * InstSize, nullptr);
        BufferOffset afterPool = this->nextOffset();
        Asm::WritePoolGuard(branch, this->getInst(branch), afterPool);

        
        
        
        BufferSlice *perforatedSlice = getTail();
        BufferOffset perforation = this->nextOffset();
        Parent::perforate();
        perforatedSlice->isNatural = false;
        JitSpew(JitSpew_Pools, "[%d] Adding a perforation at offset %d", id, perforation.getOffset());

        
        
        
        size_t poolOffset = perforation.getOffset();
        
        PoolInfo pi = getLastPoolInfo();
        size_t magicAlign = pi.finalPos - pi.offset;
        
        poolOffset += magicAlign;
        
        poolOffset += headerSize_ * InstSize;

        unsigned idx = 0;
        for (BufferOffset *iter = pool_.loadOffsets.begin();
             iter != pool_.loadOffsets.end();
             ++iter, ++idx)
        {
            
            MOZ_ASSERT(iter->getOffset() < perforation.getOffset());

            
            
            Inst *inst = this->getInst(*iter);
            size_t codeOffset = poolOffset - iter->getOffset();

            
            
            
            
            JitSpew(JitSpew_Pools, "[%d] Fixing entry %d offset to %u", id, idx,
                    codeOffset - magicAlign);
            Asm::PatchConstantPoolLoad(inst, (uint8_t *)inst + codeOffset - magicAlign);
        }

        
        if (numDumps_ >= poolInfoSize_) {
            
            poolInfoSize_ *= 2;
            PoolInfo *tmp = this->lifoAlloc_.template newArrayUninitialized<PoolInfo>(poolInfoSize_);
            if (tmp == nullptr) {
                this->fail_oom();
                return;
            }
            mozilla::PodCopy(tmp, poolInfo_, numDumps_);
            poolInfo_ = tmp;
        }
        PoolInfo newPoolInfo = getPoolData(perforatedSlice, perforation.getOffset());
        MOZ_ASSERT(numDumps_ < poolInfoSize_);
        poolInfo_[numDumps_] = newPoolInfo;
        numDumps_++;

        
        
        Pool **tmp = &perforatedSlice->pool;
        *tmp = static_cast<Pool *>(this->lifoAlloc_.alloc(sizeof(Pool)));
        if (tmp == nullptr) {
            this->fail_oom();
            return;
        }
        mozilla::PodCopy(*tmp, &pool_, 1);

        
        if (!pool_.reset(this->lifoAlloc_)) {
            this->fail_oom();
            return;
        }
    }

  public:
    void flushPool() {
        if (this->oom())
            return;
        JitSpew(JitSpew_Pools, "[%d] Requesting a pool flush", id);
        finishPool();
    }

    void enterNoPool(size_t maxInst) {
        
        MOZ_ASSERT(!canNotPlacePool_);
        insertNopFill();

        
        
        
        
        size_t poolOffset = sizeExcludingCurrentPool() + (maxInst + guardSize_ + headerSize_) * InstSize;

        if (pool_.checkFull(poolOffset)) {
            JitSpew(JitSpew_Pools, "[%d] No-Pool instruction(%d) caused a spill.", id,
                    sizeExcludingCurrentPool());
            finishPool();
        }

#ifdef DEBUG
        
        
        canNotPlacePoolStartOffset_ = this->nextOffset().getOffset();
        canNotPlacePoolMaxInst_ = maxInst;
#endif

        canNotPlacePool_ = true;
    }

    void leaveNoPool() {
        MOZ_ASSERT(canNotPlacePool_);
        canNotPlacePool_ = false;

        
        MOZ_ASSERT(this->nextOffset().getOffset() - canNotPlacePoolStartOffset_ <= canNotPlacePoolMaxInst_ * InstSize);
    }

    size_t poolSizeBefore(size_t offset) const {
        
        
        
        
        unsigned cur = 0;
        while (cur < numDumps_ && poolInfo_[cur].offset <= offset)
            cur++;
        if (cur == 0)
            return 0;
        return poolInfo_[cur - 1].finalPos - poolInfo_[cur - 1].offset;
    }

    void align(unsigned alignment)
    {
        
        MOZ_ASSERT(IsPowerOfTwo(alignment));

        
        insertNopFill();

        
        unsigned requiredFill = sizeExcludingCurrentPool() & (alignment - 1);
        if (requiredFill == 0)
            return;
        requiredFill = alignment - requiredFill;

        
        
        uint32_t poolOffset = sizeExcludingCurrentPool() + requiredFill
                              + (1 + guardSize_ + headerSize_) * InstSize;
        if (pool_.checkFull(poolOffset)) {
            
            JitSpew(JitSpew_Pools, "[%d] Alignment of %d at %d caused a spill.", id, alignment,
                    sizeExcludingCurrentPool());
            finishPool();
        }

        inhibitNops_ = true;
        while (sizeExcludingCurrentPool() & (alignment - 1))
            putInt(alignFillInst_);
        inhibitNops_ = false;
    }

  private:
    void patchBranch(Inst *i, unsigned curpool, BufferOffset branch) {
        const Inst *ci = i;
        ptrdiff_t offset = Asm::GetBranchOffset(ci);
        
        if (offset == 0)
            return;
        unsigned destOffset = branch.getOffset() + offset;
        if (offset > 0) {
            while (curpool < numDumps_ && poolInfo_[curpool].offset <= destOffset) {
                offset += poolInfo_[curpool].size;
                curpool++;
            }
        } else {
            
            
            for (int p = curpool - 1; p >= 0 && poolInfo_[p].offset > destOffset; p--)
                offset -= poolInfo_[p].size;
        }
        Asm::RetargetNearBranch(i, offset, false);
    }

  public:
    void executableCopy(uint8_t *dest_) {
        if (this->oom())
            return;
        
        MOZ_ASSERT(pool_.numEntries() == 0);
        
        MOZ_ASSERT(uintptr_t(dest_) == ((uintptr_t(dest_) + instBufferAlign_ - 1) & ~(instBufferAlign_ - 1)));
        
        static_assert(InstSize == sizeof(uint32_t), "Assuming instruction size is 4 bytes");
        uint32_t *dest = (uint32_t *)dest_;
        unsigned curIndex = 0;
        size_t curInstOffset = 0;
        for (BufferSlice *cur = getHead(); cur != nullptr; cur = cur->getNext()) {
            uint32_t *src = (uint32_t *)&cur->instructions;
            unsigned numInsts = cur->size() / InstSize;
            for (unsigned idx = 0; idx < numInsts; idx++, curInstOffset += InstSize) {
                
                if (cur->isBranch(idx)) {
                    
                    patchBranch((Inst *)&src[idx], curIndex, BufferOffset(curInstOffset));
                }
                dest[idx] = src[idx];
            }
            dest += numInsts;
            Pool *curPool = cur->pool;
            if (curPool != nullptr) {
                
                curIndex++;
                
                uint8_t *poolDest = (uint8_t *)dest;
                Asm::WritePoolHeader(poolDest, curPool, cur->isNatural);
                poolDest += headerSize_ * InstSize;

                memcpy(poolDest, curPool->poolData(), curPool->getPoolSize());
                poolDest += curPool->getPoolSize();

                dest = (uint32_t *)poolDest;
            }
        }
    }

  public:
    size_t poolEntryOffset(PoolEntry pe) const {
        
        
        
        size_t offset = pe.index() * sizeof(PoolAllocUnit);
        for (unsigned poolNum = 0; poolNum < numDumps_; poolNum++) {
            PoolInfo *pi = &poolInfo_[poolNum];
            unsigned size = pi->slice->pool->getPoolSize();
            if (size > offset)
                return pi->finalPos - pi->size + headerSize_ * InstSize + offset;
            offset -= size;
        }
        MOZ_CRASH("Entry is not in a pool");
    }
};

} 
} 
#endif 
