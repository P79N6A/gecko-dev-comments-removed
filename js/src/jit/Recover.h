





#ifndef jit_Recover_h
#define jit_Recover_h

#include "mozilla/Attributes.h"

#include "jit/Snapshots.h"

struct JSContext;

namespace js {
namespace jit {

#define RECOVER_OPCODE_LIST(_)                  \
    _(ResumePoint)                              \
    _(BitNot)                                   \
    _(BitAnd)                                   \
    _(BitOr)                                    \
    _(BitXor)                                   \
    _(Lsh)                                      \
    _(Rsh)                                      \
    _(Ursh)                                     \
    _(Add)                                      \
    _(Sub)                                      \
    _(Mul)                                      \
    _(Div)                                      \
    _(Mod)                                      \
    _(Not)                                      \
    _(Concat)                                   \
    _(StringLength)                             \
    _(ArgumentsLength)                          \
    _(Floor)                                    \
    _(Round)                                    \
    _(CharCodeAt)                               \
    _(FromCharCode)                             \
    _(Pow)                                      \
    _(PowHalf)                                  \
    _(MinMax)                                   \
    _(Abs)                                      \
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
    explicit R##op(CompactBufferReader &reader);                        \
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

class RBitAnd MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(BitAnd)

    virtual uint32_t numOperands() const {
        return 2;
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

class RBitXor MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(BitXor)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RLsh MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Lsh)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RRsh MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Rsh)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RUrsh MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Ursh)

    virtual uint32_t numOperands() const {
        return 2;
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

class RSub MOZ_FINAL : public RInstruction
{
  private:
    bool isFloatOperation_;

  public:
    RINSTRUCTION_HEADER_(Sub)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RMul MOZ_FINAL : public RInstruction
{
  private:
    bool isFloatOperation_;

  public:
    RINSTRUCTION_HEADER_(Mul)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RDiv MOZ_FINAL : public RInstruction
{
  private:
    bool isFloatOperation_;

  public:
    RINSTRUCTION_HEADER_(Div)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RMod MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Mod)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RNot MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Not)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RConcat MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Concat)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RStringLength MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(StringLength)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RArgumentsLength MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(ArgumentsLength)

    virtual uint32_t numOperands() const {
        return 0;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};


class RFloor MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Floor)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RRound MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Round)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RCharCodeAt MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(CharCodeAt)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RFromCharCode MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(FromCharCode)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RPow MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Pow)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RPowHalf MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(PowHalf)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RMinMax MOZ_FINAL : public RInstruction
{
  private:
    bool isMax_;

  public:
    RINSTRUCTION_HEADER_(MinMax)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RAbs MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Abs)

    virtual uint32_t numOperands() const {
        return 1;
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
