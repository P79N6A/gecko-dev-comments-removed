





#ifndef jit_shared_IonAssemblerBuffer_h
#define jit_shared_IonAssemblerBuffer_h


#include "jit/shared/Assembler-shared.h"

namespace js {
namespace jit {







class BufferOffset
{
    int offset;
  public:
    friend BufferOffset nextOffset();
    explicit BufferOffset(int offset_) : offset(offset_) {}
    
    int getOffset() const { return offset; }

    
    
    
    
    
    template <class BOffImm>
    BOffImm diffB(BufferOffset other) const {
        return BOffImm(offset - other.offset);
    }

    template <class BOffImm>
    BOffImm diffB(Label *other) const {
        MOZ_ASSERT(other->bound());
        return BOffImm(offset - other->offset());
    }

    explicit BufferOffset(Label *l) : offset(l->offset()) {
    }
    explicit BufferOffset(RepatchLabel *l) : offset(l->offset()) {
    }

    BufferOffset() : offset(INT_MIN) {}
    bool assigned() const { return offset != INT_MIN; }
};

template<int SliceSize>
struct BufferSlice {
  protected:
    BufferSlice<SliceSize> *prev_;
    BufferSlice<SliceSize> *next_;
    
    uint32_t nodeSize_;
  public:
    BufferSlice *getNext() const { return next_; }
    BufferSlice *getPrev() const { return prev_; }
    void setNext(BufferSlice<SliceSize> *next) {
        MOZ_ASSERT(next_ == nullptr);
        MOZ_ASSERT(next->prev_ == nullptr);
        next_ = next;
        next->prev_ = this;
    }

    mozilla::Array<uint8_t, SliceSize> instructions;
    size_t size() const {
        return nodeSize_;
    }
    explicit BufferSlice() : prev_(nullptr), next_(nullptr), nodeSize_(0) {}
    void putBlob(uint32_t instSize, uint8_t* inst) {
        if (inst != nullptr)
            memcpy(&instructions[size()], inst, instSize);
        nodeSize_ += instSize;
    }
};

template<int SliceSize, class Inst>
struct AssemblerBuffer
{
  public:
    explicit AssemblerBuffer() : head(nullptr), tail(nullptr), m_oom(false),
                                 m_bail(false), bufferSize(0), lifoAlloc_(8192) {}
  protected:
    typedef BufferSlice<SliceSize> Slice;
    typedef AssemblerBuffer<SliceSize, Inst> AssemblerBuffer_;
    Slice *head;
    Slice *tail;
  public:
    bool m_oom;
    bool m_bail;
    
    uint32_t bufferSize;
    uint32_t lastInstSize;
    bool isAligned(int alignment) const {
        
        MOZ_ASSERT(IsPowerOfTwo(alignment));
        return !(size() & (alignment - 1));
    }
    virtual Slice *newSlice(LifoAlloc &a) {
        Slice *tmp = static_cast<Slice*>(a.alloc(sizeof(Slice)));
        if (!tmp) {
            m_oom = true;
            return nullptr;
        }
        new (tmp) Slice;
        return tmp;
    }
    bool ensureSpace(int size) {
        if (tail != nullptr && tail->size() + size <= SliceSize)
            return true;
        Slice *tmp = newSlice(lifoAlloc_);
        if (tmp == nullptr)
            return false;
        if (tail != nullptr) {
            bufferSize += tail->size();
            tail->setNext(tmp);
        }
        tail = tmp;
        if (head == nullptr) {
            finger = tmp;
            finger_offset = 0;
            head = tmp;
        }
        return true;
    }

    BufferOffset putByte(uint8_t value) {
        return putBlob(sizeof(value), (uint8_t*)&value);
    }

    BufferOffset putShort(uint16_t value) {
        return putBlob(sizeof(value), (uint8_t*)&value);
    }

    BufferOffset putInt(uint32_t value) {
        return putBlob(sizeof(value), (uint8_t*)&value);
    }
    BufferOffset putBlob(uint32_t instSize, uint8_t *inst) {
        if (!ensureSpace(instSize))
            return BufferOffset();
        BufferOffset ret = nextOffset();
        tail->putBlob(instSize, inst);
        return ret;
    }
    unsigned int size() const {
        int executableSize;
        if (tail != nullptr)
            executableSize = bufferSize + tail->size();
        else
            executableSize = bufferSize;
        return executableSize;
    }
    bool oom() const {
        return m_oom || m_bail;
    }
    bool bail() const {
        return m_bail;
    }
    void fail_oom() {
        m_oom = true;
    }
    void fail_bail() {
        m_bail = true;
    }
    
    Slice *finger;
    unsigned int finger_offset;
    Inst *getInst(BufferOffset off) {
        int local_off = off.getOffset();
        
        
        Slice *cur = nullptr;
        int cur_off;
        
        
        int end_off = bufferSize - local_off;
        
        
        if (end_off <= 0)
            return (Inst*)&tail->instructions[-end_off];
        bool used_finger = false;
        int finger_off = abs((int)(local_off - finger_offset));
        if (finger_off < Min(local_off, end_off)) {
            
            cur = finger;
            cur_off = finger_offset;
            used_finger = true;
        } else if (local_off < end_off) {
            
            cur = head;
            cur_off = 0;
        } else {
            
            cur = tail;
            cur_off = bufferSize;
        }
        int count = 0;
        if (local_off < cur_off) {
            for (; cur != nullptr; cur = cur->getPrev(), cur_off -= cur->size()) {
                if (local_off >= cur_off) {
                    local_off -= cur_off;
                    break;
                }
                count++;
            }
            MOZ_ASSERT(cur != nullptr);
        } else {
            for (; cur != nullptr; cur = cur->getNext()) {
                int cur_size = cur->size();
                if (local_off < cur_off + cur_size) {
                    local_off -= cur_off;
                    break;
                }
                cur_off += cur_size;
                count++;
            }
            MOZ_ASSERT(cur != nullptr);
        }
        if (count > 2 || used_finger) {
            finger = cur;
            finger_offset = cur_off;
        }
        
        
        MOZ_ASSERT(local_off < (int)cur->size());
        return (Inst*)&cur->instructions[local_off];
    }
    BufferOffset nextOffset() const {
        if (tail != nullptr)
            return BufferOffset(bufferSize + tail->size());
        else
            return BufferOffset(bufferSize);
    }
    BufferOffset prevOffset() const {
        MOZ_CRASH("Don't current record lastInstSize");
    }

    
    void perforate() {
        Slice *tmp = newSlice(lifoAlloc_);
        if (!tmp) {
            m_oom = true;
            return;
        }
        bufferSize += tail->size();
        tail->setNext(tmp);
        tail = tmp;
    }

    void executableCopy(uint8_t *dest_) {
        if (this->oom())
            return;

        for (Slice *cur = head; cur != nullptr; cur = cur->getNext()) {
            memcpy(dest_, &cur->instructions, cur->size());
            dest_ += cur->size();
        }
    }

    class AssemblerBufferInstIterator {
      private:
        BufferOffset bo;
        AssemblerBuffer_ *m_buffer;
      public:
        explicit AssemblerBufferInstIterator(BufferOffset off, AssemblerBuffer_ *buff)
            : bo(off), m_buffer(buff)
        {
        }
        Inst *next() {
            Inst *i = m_buffer->getInst(bo);
            bo = BufferOffset(bo.getOffset() + i->size());
            return cur();
        }
        Inst *cur() {
            return m_buffer->getInst(bo);
        }
    };
  public:
    LifoAlloc lifoAlloc_;
};

} 
} 
#endif 
