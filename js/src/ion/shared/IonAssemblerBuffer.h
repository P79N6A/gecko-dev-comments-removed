








































#ifndef __ion_assembler_buffer_h
#define __ion_assembler_buffer_h

#include "ion/shared/Assembler-shared.h"

namespace js {
namespace ion {





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
        JS_ASSERT(other->bound());
        return BOffImm(offset - other->offset());
    }

    explicit BufferOffset(Label *l) : offset(l->offset()) {
    }

    BufferOffset() : offset(INT_MIN) {}
    bool assigned() const { return offset != INT_MIN; };
};

template<int SliceSize>
struct BufferSlice : public InlineForwardListNode<BufferSlice<SliceSize> > {
  protected:
    
    uint32 nodeSize;
  public:
    BufferSlice *getNext() { return static_cast<BufferSlice *>(this->next); }
    void setNext(BufferSlice<SliceSize> *next_) {
        JS_ASSERT(this->next == NULL);
        this->next = next_;
    }
    uint8 instructions [SliceSize];
    unsigned int size() {
        return nodeSize;
    }
    BufferSlice() : InlineForwardListNode<BufferSlice<SliceSize> >(NULL), nodeSize(0) {}
    void putBlob(uint32 instSize, uint8* inst) {
        if (inst != NULL)
            memcpy(&instructions[size()], inst, instSize);
        nodeSize += instSize;
    }
};

template<int SliceSize, class Inst>
struct AssemblerBuffer {
  public:
    AssemblerBuffer() : head(NULL), tail(NULL), m_oom(false), bufferSize(0) {}
  protected:
    typedef BufferSlice<SliceSize> Slice;
    Slice *head;
    Slice *tail;
  public:
    bool m_oom;
    
    uint32 bufferSize;
    uint32 lastInstSize;
    bool isAligned(int alignment) const {
        
        JS_ASSERT((alignment & (alignment-1)) == 0);
        return !(size() & (alignment - 1));
    }
    virtual Slice *newSlice() {
        Slice *tmp = static_cast<Slice*>(malloc(sizeof(Slice)));
        if (!tmp) {
            m_oom = true;
            return NULL;
        }
        new (tmp) Slice;
        return tmp;
    }
    bool ensureSpace(int size) {
        if (tail != NULL && tail->size()+size <= SliceSize)
            return true;
        Slice *tmp = newSlice();
        if (tmp == NULL)
            return false;
        if (tail != NULL) {
            bufferSize += tail->size();
            tail->setNext(tmp);
        }
        tail = tmp;
        if (head == NULL)
            head = tmp;
        return true;
    }

    void putByte(uint8 value) {
        putBlob(sizeof(value), (uint8*)&value);
    }

    void putShort(uint16 value) {
        putBlob(sizeof(value), (uint8*)&value);
    }

    void putInt(uint32 value) {
        putBlob(sizeof(value), (uint8*)&value);
    }
    void putBlob(uint32 instSize, uint8 *inst) {
        if (!ensureSpace(instSize))
            return;
        tail->putBlob(instSize, inst);
    }
    unsigned int size() const {
        int executableSize;
        if (tail != NULL)
            executableSize = bufferSize + tail->size();
        else
            executableSize = bufferSize;
        return executableSize;
    }
    unsigned int uncheckedSize() const {
        return size();
    }
    bool oom() const {
        return m_oom;
    }
    void fail_oom() {
        m_oom = true;
    }
    Inst *getInst(BufferOffset off) {
        unsigned int local_off = off.getOffset();
        Slice *cur = NULL;
        if (local_off > bufferSize) {
            local_off -= bufferSize;
            cur = tail;
        } else {
            for (cur = head; cur != NULL; cur = cur->getNext()) {
                if (local_off < cur->size())
                    break;
                local_off -= cur->size();
            }
            JS_ASSERT(cur != NULL);
        }
        
        JS_ASSERT(local_off < cur->size());
        return (Inst*)&cur->instructions[local_off];
    }
    BufferOffset nextOffset() const {
        if (tail != NULL)
            return BufferOffset(bufferSize + tail->size());
        else
            return BufferOffset(bufferSize);
    }
    BufferOffset prevOffset() const {
        JS_NOT_REACHED("Don't current record lastInstSize");
        return BufferOffset(bufferSize + tail->nodeSize - lastInstSize);
    }

    
    void perforate() {
        Slice *tmp = newSlice();
        if (!tmp)
            m_oom = true;
        bufferSize += tail->size();
        tail->setNext(tmp);
        tail = tmp;
    }

};

} 
} 

#endif 
