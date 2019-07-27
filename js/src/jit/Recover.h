





#ifndef jit_Recover_h
#define jit_Recover_h

#include "mozilla/Attributes.h"

#include "jsarray.h"

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
    _(Sqrt)                                     \
    _(Atan2)                                    \
    _(StringSplit)                              \
    _(RegExpExec)                               \
    _(RegExpTest)                               \
    _(RegExpReplace)                            \
    _(TypeOf)                                   \
    _(ToFloat32)                                \
    _(NewObject)                                \
    _(NewArray)                                 \
    _(NewDerivedTypedObject)                    \
    _(CreateThisWithTemplate)                   \
    _(ObjectState)                              \
    _(ArrayState)

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

class RSqrt MOZ_FINAL : public RInstruction
{
  private:
    bool isFloatOperation_;

  public:
    RINSTRUCTION_HEADER_(Sqrt)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RAtan2 MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(Atan2)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RStringSplit MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(StringSplit)

    virtual uint32_t numOperands() const {
        return 3;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RRegExpExec MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(RegExpExec)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RRegExpTest MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(RegExpTest)

    virtual uint32_t numOperands() const {
        return 2;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RRegExpReplace MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(RegExpReplace)

    virtual uint32_t numOperands() const {
        return 3;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RTypeOf MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(TypeOf)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RToFloat32 MOZ_FINAL : public RInstruction
{
  public:
    RINSTRUCTION_HEADER_(ToFloat32)

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

class RNewArray MOZ_FINAL : public RInstruction
{
  private:
    uint32_t count_;
    AllocatingBehaviour allocatingBehaviour_;

  public:
    RINSTRUCTION_HEADER_(NewArray)

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

class RCreateThisWithTemplate MOZ_FINAL : public RInstruction
{
  private:
    bool tenuredHeap_;

  public:
    RINSTRUCTION_HEADER_(CreateThisWithTemplate)

    virtual uint32_t numOperands() const {
        return 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RObjectState MOZ_FINAL : public RInstruction
{
  private:
    uint32_t numSlots_;        

  public:
    RINSTRUCTION_HEADER_(ObjectState)

    uint32_t numSlots() const {
        return numSlots_;
    }
    virtual uint32_t numOperands() const {
        
        return numSlots() + 1;
    }

    bool recover(JSContext *cx, SnapshotIterator &iter) const;
};

class RArrayState MOZ_FINAL : public RInstruction
{
  private:
    uint32_t numElements_;

  public:
    RINSTRUCTION_HEADER_(ArrayState)

    uint32_t numElements() const {
        return numElements_;
    }
    virtual uint32_t numOperands() const {
        
        
        return numElements() + 2;
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
