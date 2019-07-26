





#ifndef jit_Recover_h
#define jit_Recover_h

#include "mozilla/Attributes.h"

#include "jit/Snapshots.h"

class JSContext;

namespace js {
namespace jit {

#define RECOVER_OPCODE_LIST(_)                  \
    _(ResumePoint)                              \
    _(BitNot)                                   \
    _(BitOr)                                    \
    _(Add)                                      \
    _(NewObject)                                \
    _(NewDerivedTypedObject)

class RResumePoint;
class SnapshotIterator;

class RInstruction
{
  public:
    enum Opcode
    {
#   define DEFINE_OPCODES_(op) Recover_##op,
        RECOVER_OPCODE_LIST(DEFINE_OPCODES_)
#   undef DEFINE_OPCODES_
        Recover_Invalid
    };

    virtual Opcode opcode() const = 0;

    
    
    bool isResumePoint() const {
        return opcode() == Recover_ResumePoint;
    }
    inline const RResumePoint *toResumePoint() const;

    
    
    virtual uint32_t numOperands() const = 0;

    
    
    
    virtual bool recover(JSContext *cx, SnapshotIterator &iter) const = 0;

    
    
    
    static void readRecoverData(CompactBufferReader &reader, RInstructionStorage *raw);
};

#define RINSTRUCTION_HEADER_(op)                                        \
  private:                                                              \
    friend class RInstruction;                                          \
    R##op(CompactBufferReader &reader);                                 \
                                                                        \
  public:                                                               \
    Opcode opcode() const {                                             \
        return RInstruction::Recover_##op;                              \
    }

class RResumePoint MOZ_FINAL : public RInstruction
{
  private:
    uint32_t pcOffset_;           
    uint32_t numOperands_;        

  public:
    RINSTRUCTION_HEADER_(ResumePoint)

    uint32_t pcOffset() const {
        return pcOffset_;
    }
    virtual uint32_t numOperands() const {
        return numOperands_;
    }
    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RBitNot MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(BitNot)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RAdd MOZ_FINAL : public RInstruction
{
  private:
    bool isFloatOperation_;

  public:
    RINSTRUCTION_HEADER_(Add)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RNewObject MOZ_FINAL : public RInstruction
{
  private:
    bool templateObjectIsClassPrototype_;

  public:
    RINSTRUCTION_HEADER_(NewObject)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RNewDerivedTypedObject MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(NewDerivedTypedObject)

    virtual uint32_t numOperands() const {
        return 3;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RBitOr MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(BitOr)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

#undef RINSTRUCTION_HEADER_

const RResumePoint *
RInstruction::toResumePoint() const
{
    MOZ_ASSERT(isResumePoint());
    return static_cast<const RResumePoint *>(this);
}

}
}

#endif 
