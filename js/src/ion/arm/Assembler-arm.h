








































#ifndef jsion_cpu_arm_assembler_h__
#define jsion_cpu_arm_assembler_h__

#include "ion/shared/Assembler-shared.h"
#include "assembler/assembler/AssemblerBufferWithConstantPool.h"
#include "ion/CompactBuffer.h"
#include "ion/IonCode.h"

namespace js {
namespace ion {







static const Register r0  = { Registers::r0 };
static const Register r1  = { Registers::r1 };
static const Register r2  = { Registers::r2 };
static const Register r3  = { Registers::r3 };
static const Register r4  = { Registers::r4 };
static const Register r5  = { Registers::r5 };
static const Register r6  = { Registers::r6 };
static const Register r7  = { Registers::r7 };
static const Register r8  = { Registers::r8 };
static const Register r9  = { Registers::r9 };
static const Register r10 = { Registers::r10 };
static const Register r11 = { Registers::r11 };
static const Register r12 = { Registers::ip };
static const Register ip  = { Registers::ip };
static const Register sp  = { Registers::sp };
static const Register r14 = { Registers::lr };
static const Register lr  = { Registers::lr };
static const Register pc  = { Registers::pc };

static const Register ScratchRegister = {Registers::ip};

static const Register InvalidReg = { Registers::invalid_reg };
static const FloatRegister InvalidFloatReg = { FloatRegisters::invalid_freg };

static const Register JSReturnReg_Type = r3;
static const Register JSReturnReg_Data = r2;
static const Register StackPointer = sp;
static const Register ReturnReg = r0;
static const FloatRegister ScratchFloatReg = { FloatRegisters::d0 };

static const FloatRegister d0  = {FloatRegisters::d0};
static const FloatRegister d1  = {FloatRegisters::d1};
static const FloatRegister d2  = {FloatRegisters::d2};
static const FloatRegister d3  = {FloatRegisters::d3};
static const FloatRegister d4  = {FloatRegisters::d4};
static const FloatRegister d5  = {FloatRegisters::d5};
static const FloatRegister d6  = {FloatRegisters::d6};
static const FloatRegister d7  = {FloatRegisters::d7};
static const FloatRegister d8  = {FloatRegisters::d8};
static const FloatRegister d9  = {FloatRegisters::d9};
static const FloatRegister d10 = {FloatRegisters::d10};
static const FloatRegister d11 = {FloatRegisters::d11};
static const FloatRegister d12 = {FloatRegisters::d12};
static const FloatRegister d13 = {FloatRegisters::d13};
static const FloatRegister d14 = {FloatRegisters::d14};
static const FloatRegister d15 = {FloatRegisters::d15};

uint32 RM(Register r);
uint32 RS(Register r);
uint32 RD(Register r);
uint32 RT(Register r);
uint32 RN(Register r);

class VFPRegister;
uint32 VD(VFPRegister vr);
uint32 VN(VFPRegister vr);
uint32 VM(VFPRegister vr);

class VFPRegister
{
  public:
    
    
    
    enum RegType {
        Double = 0x0,
        Single = 0x1,
        UInt   = 0x2,
        Int    = 0x3
    };
  private:
    RegType kind:2;
    
    
    
    
    
    
    
    int _code:5;
    bool _isInvalid:1;
    bool _isMissing:1;
    VFPRegister(int  r, RegType k)
        : kind(k), _code (r), _isInvalid(false), _isMissing(false) {}
  public:
    VFPRegister()
        : _isInvalid(true), _isMissing(false) {}
    VFPRegister(bool b)
        : _isInvalid(false), _isMissing(b) {}
    VFPRegister(FloatRegister fr)
        : kind(Double), _code(fr.code()), _isInvalid(false), _isMissing(false)
    {
        JS_ASSERT(_code == fr.code());
    }
    VFPRegister(FloatRegister fr, RegType k)
        : kind(k), _code (fr.code())
    {
        JS_ASSERT(_code == fr.code());
    }
    bool isDouble() { return kind == Double; }
    bool isSingle() { return kind == Single; }
    bool isFloat() { return (kind == Double) || (kind == Single); }
    bool isInt() { return (kind == UInt) || (kind == Int); }
    bool isSInt()   { return kind == Int; }
    bool isUInt()   { return kind == UInt; }
    bool equiv(VFPRegister other) { return other.kind == kind; }
    VFPRegister doubleOverlay();
    VFPRegister singleOverlay();
    VFPRegister sintOverlay();
    VFPRegister uintOverlay();
    bool isInvalid();
    bool isMissing();
    struct VFPRegIndexSplit;
    VFPRegIndexSplit encode();
    
    struct VFPRegIndexSplit {
        const uint32 block : 4;
        const uint32 bit : 1;
      private:
        friend VFPRegIndexSplit js::ion::VFPRegister::encode();
        VFPRegIndexSplit (uint32 block_, uint32 bit_)
            : block(block_), bit(bit_)
        {
            JS_ASSERT (block == block_);
            JS_ASSERT(bit == bit_);
        }
    };
    uint32 code() {
        return _code;
    }
};


extern VFPRegister NoVFPRegister;
struct ImmTag : public Imm32
{
    ImmTag(JSValueTag mask)
        : Imm32(int32(mask))
    { }
};

struct ImmType : public ImmTag
{
    ImmType(JSValueType type)
        : ImmTag(JSVAL_TYPE_TO_TAG(type))
    { }
};

enum Index {
    Offset = 0 << 21 | 1<<24,
    PreIndex = 1<<21 | 1 << 24,
    PostIndex = 0 << 21 | 0 << 24
    
    
};


enum IsImmOp2_ {
    IsImmOp2    = 1 << 25,
    IsNotImmOp2 = 0 << 25
};
enum IsImmDTR_ {
    IsImmDTR    = 0 << 25,
    IsNotImmDTR = 1 << 25
};

enum IsImmEDTR_ {
    IsImmEDTR    = 1 << 22,
    IsNotImmEDTR = 0 << 22
};


enum ShiftType {
    LSL = 0, 
    LSR = 1, 
    ASR = 2, 
    ROR = 3, 
    RRX = ROR 
};



struct ConditionCodes
{
    bool Zero : 1;
    bool Overflow : 1;
    bool Carry : 1;
    bool Minus : 1;
};




enum DTMMode {
    A = 0 << 24, 
    B = 1 << 24, 
    D = 0 << 23, 
    I = 1 << 23, 
    DA = D | A,
    DB = D | B,
    IA = I | A,
    IB = I | B
};

enum DTMWriteBack {
    WriteBack   = 1 << 21,
    NoWriteBack = 0 << 21
};

enum SetCond_ {
    SetCond   = 1 << 20,
    NoSetCond = 0 << 20
};
enum LoadStore {
    IsLoad  = 1 << 20,
    IsStore = 0 << 20
};




enum IsUp_ {
    IsUp   = 1 << 23,
    IsDown = 0 << 23
};
enum ALUOp {
    op_mov = 0xd << 21,
    op_mvn = 0xf << 21,
    op_and = 0x0 << 21,
    op_bic = 0xe << 21,
    op_eor = 0x1 << 21,
    op_orr = 0xc << 21,
    op_adc = 0x5 << 21,
    op_add = 0x4 << 21,
    op_sbc = 0x6 << 21,
    op_sub = 0x2 << 21,
    op_rsb = 0x3 << 21,
    op_rsc = 0x7 << 21,
    op_cmn = 0xb << 21,
    op_cmp = 0xa << 21,
    op_teq = 0x9 << 21,
    op_tst = 0x8 << 21,
    op_invalid = -1
};
enum BranchTag {
    op_b = 0x0a000000,
    op_b_mask = 0x0f000000,
    op_b_dest_mask = 0x00ffffff,
    op_bl = 0x0b000000,
    op_blx = 0x012fff30,
    op_bx  = 0x012fff10
};


enum VFPOp {
    opv_mul  = 0x2 << 20,
    opv_add  = 0x3 << 20,
    opv_sub  = 0x3 << 20 | 0x1 << 6,
    opv_div  = 0x8 << 20,
    opv_mov  = 0xB << 20 | 0x1 << 6,
    opv_abs  = 0xB << 20 | 0x3 << 6,
    opv_neg  = 0xB << 20 | 0x1 << 6 | 0x1 << 16,
    opv_sqrt = 0xB << 20 | 0x3 << 6 | 0x1 << 16,
    opv_cmp  = 0xB << 20 | 0x1 << 6 | 0x4 << 16,
    opv_cmpz  = 0xB << 20 | 0x1 << 6 | 0x5 << 16
};

ALUOp ALUNeg(ALUOp op, Imm32 *imm);
bool can_dbl(ALUOp op);
bool condsAreSafe(ALUOp op);


ALUOp getDestVariant(ALUOp op);

class ValueOperand
{
    Register type_;
    Register payload_;

  public:
    ValueOperand(Register type, Register payload)
        : type_(type), payload_(payload)
    { }

    Register typeReg() const {
        return type_;
    }
    Register payloadReg() const {
        return payload_;
    }
};




















namespace datastore {
struct Reg
{
    
    uint32 RM : 4;
    
    uint32 RRS : 1;
    ShiftType Type : 2;
    
    
    uint32 ShiftAmount : 5;
    uint32 pad : 20;
    Reg(uint32 rm, ShiftType type, uint32 rsr, uint32 shiftamount)
        : RM(rm), RRS(rsr), Type(type), ShiftAmount(shiftamount), pad(0) {}
    uint32 encode() {
        return RM | RRS << 4 | Type << 5 | ShiftAmount << 7;
    }
};




struct Imm8mData
{
  private:
    uint32 data:8;
    uint32 rot:4;
    
    
    
    uint32 buff : 19;
  public:
    uint32 invalid : 1;
    uint32 encode() {
        JS_ASSERT(!invalid);
        return data | rot << 8;
    };
    
    Imm8mData() : data(0xff), rot(0xf), invalid(1) {}
    Imm8mData(uint32 data_, uint32 rot_)
        : data(data_), rot(rot_), invalid(0)  {}
};

struct Imm8Data
{
  private:
    uint32 imm4L : 4;
    uint32 pad : 4;
    uint32 imm4H : 4;
  public:
    uint32 encode() {
        return imm4L | (imm4H << 8);
    };
    Imm8Data(uint32 imm) : imm4L(imm&0xf), imm4H(imm>>4) {
        JS_ASSERT(imm <= 0xff);
    }
};


struct Imm8VFPOffData
{
  private:
    uint32 data;
  public:
    uint32 encode() {
        return data;
    };
    Imm8VFPOffData(uint32 imm) : data (imm) {
        JS_ASSERT((imm & ~(0xff)) == 0);
    }
};


struct Imm8VFPImmData
{
  private:
    uint32 imm4L : 4;
    uint32 pad : 12;
    uint32 imm4H : 4;
    int32 isInvalid : 12;
  public:
    uint32 encode() {
        if (isInvalid != 0)
            return -1;
        return imm4L | (imm4H << 16);
    };
    Imm8VFPImmData() : imm4L(-1U & 0xf), imm4H(-1U & 0xf), isInvalid(-1) {}
    Imm8VFPImmData(uint32 imm) : imm4L(imm&0xf), imm4H(imm>>4), isInvalid(0) {
        JS_ASSERT(imm <= 0xff);
    }
};

struct Imm12Data
{
    uint32 data : 12;
    Imm12Data(uint32 imm) : data(imm) { JS_ASSERT(data == imm); }
    uint32 encode() { return data; }
};

struct RIS
{
    uint32 ShiftAmount : 5;
    RIS(uint32 imm) : ShiftAmount(imm) { ASSERT(ShiftAmount == imm); }
    uint32 encode () {
        return ShiftAmount;
    }
};
struct RRS
{
    uint32 MustZero : 1;
    
    uint32 RS : 4;
    RRS(uint32 rs) : RS(rs) { ASSERT(rs == RS); }
    uint32 encode () {return RS << 1;}
};

} 


class MacroAssemblerARM;
class Operand;
class Operand2
{
  public:
    uint32 oper : 31;
    uint32 invalid : 1;
    
  protected:
    friend class MacroAssemblerARM;
    Operand2(datastore::Imm8mData base)
        : oper(base.invalid ? -1 : (base.encode() | (uint32)IsImmOp2)),
          invalid(base.invalid)
    {
    }
    Operand2(datastore::Reg base) : oper(base.encode() | (uint32)IsNotImmOp2) {}
  private:
    friend class Operand;
    Operand2(int blob) : oper(blob) {}
  public:
    uint32 encode() { return oper; }
};

class Imm8 : public Operand2
{
  public:
    static datastore::Imm8mData encodeImm(uint32 imm) {
        
        if (imm == 0)
            return datastore::Imm8mData(0, 0);
        int left = js_bitscan_clz32(imm) & 30;
        
        
        
        if (left >= 24)
            return datastore::Imm8mData(imm, 0);

        
        
        int no_imm = imm & ~(0xff << (24 - left));
        if (no_imm == 0) {
            return  datastore::Imm8mData(imm >> (24 - left), ((8+left) >> 1));
        }
        
        int right = 32 - (js_bitscan_clz32(no_imm)  & 30);
        
        
        if (right >= 8)
            return datastore::Imm8mData();
        
        
        unsigned int mask = imm << (8 - right) | imm >> (24 + right);
        if (mask <= 0xff)
            return datastore::Imm8mData(mask, (8-right) >> 1);
        return datastore::Imm8mData();
    }
    
    struct TwoImm8mData
    {
        datastore::Imm8mData fst, snd;
        TwoImm8mData() : fst(), snd() {}
        TwoImm8mData(datastore::Imm8mData _fst, datastore::Imm8mData _snd)
            : fst(_fst), snd(_snd) {}
    };
    static TwoImm8mData encodeTwoImms(uint32);
    Imm8(uint32 imm) : Operand2(encodeImm(imm)) {}
};

class Op2Reg : public Operand2
{
  protected:
    Op2Reg(Register rm, ShiftType type, datastore::RIS shiftImm)
        : Operand2(datastore::Reg(rm.code(), type, 0, shiftImm.encode())) {}
    Op2Reg(Register rm, ShiftType type, datastore::RRS shiftReg)
        : Operand2(datastore::Reg(rm.code(), type, 1, shiftReg.encode())) {}
};
class O2RegImmShift : public Op2Reg
{
  public:
    O2RegImmShift(Register rn, ShiftType type, uint32 shift)
        : Op2Reg(rn, type, datastore::RIS(shift)) {}
};

class O2RegRegShift : public Op2Reg
{
  public:
    O2RegRegShift(Register rn, ShiftType type, Register rs)
        : Op2Reg(rn, type, datastore::RRS(rs.code())) {}
};
O2RegImmShift O2Reg(Register r);
O2RegImmShift lsl (Register r, int amt);
O2RegImmShift lsr (Register r, int amt);
O2RegImmShift asr (Register r, int amt);
O2RegImmShift rol (Register r, int amt);
O2RegImmShift ror (Register r, int amt);

O2RegRegShift lsl (Register r, Register amt);
O2RegRegShift lsr (Register r, Register amt);
O2RegRegShift asr (Register r, Register amt);
O2RegRegShift ror (Register r, Register amt);






class DtrOff
{
    uint32 data;
  protected:
    DtrOff(datastore::Imm12Data immdata, IsUp_ iu)
    : data(immdata.encode() | (uint32)IsImmDTR | ((uint32)iu)) {}
    DtrOff(datastore::Reg reg, IsUp_ iu = IsUp)
        : data(reg.encode() | (uint32) IsNotImmDTR | iu) {}
  public:
    uint32 encode() { return data; }
};

class DtrOffImm : public DtrOff
{
  public:
    DtrOffImm(int32 imm)
        : DtrOff(datastore::Imm12Data(abs(imm)), imm >= 0 ? IsUp : IsDown)
    { JS_ASSERT((imm < 4096) && (imm > -4096)); }
};

class DtrOffReg : public DtrOff
{
    
    
  protected:
    DtrOffReg(Register rn, ShiftType type, datastore::RIS shiftImm)
        : DtrOff(datastore::Reg(rn.code(), type, 0, shiftImm.encode())) {}
    DtrOffReg(Register rn, ShiftType type, datastore::RRS shiftReg)
        : DtrOff(datastore::Reg(rn.code(), type, 1, shiftReg.encode())) {}
};

class DtrRegImmShift : public DtrOffReg
{
  public:
    DtrRegImmShift(Register rn, ShiftType type, uint32 shift)
        : DtrOffReg(rn, type, datastore::RIS(shift)) {}
};

class DtrRegRegShift : public DtrOffReg
{
  public:
    DtrRegRegShift(Register rn, ShiftType type, Register rs)
        : DtrOffReg(rn, type, datastore::RRS(rs.code())) {}
};



class DTRAddr
{
    uint32 data;
  public:
    DTRAddr(Register reg, DtrOff dtr)
        : data(dtr.encode() | (reg.code() << 16)) {}
    uint32 encode() { return data; }
  private:
    friend class Operand;
    DTRAddr(uint32 blob) : data(blob) {}
};



class EDtrOff
{
  protected:
    uint32 data;
    EDtrOff(datastore::Imm8Data imm8, IsUp_ iu = IsUp)
        : data(imm8.encode() | IsImmEDTR | (uint32)iu) {}
    EDtrOff(Register rm, IsUp_ iu = IsUp)
        : data(rm.code() | IsNotImmEDTR | iu) {}
  public:
    uint32 encode() { return data; }
};

class EDtrOffImm : public EDtrOff
{
  public:
    EDtrOffImm(uint32 imm)
        : EDtrOff(datastore::Imm8Data(abs(imm)), (imm >= 0) ? IsUp : IsDown) {}
};




class EDtrOffReg : EDtrOff
{
  public:
    EDtrOffReg(Register rm) : EDtrOff(rm) {}
};

class EDtrAddr
{
    uint32 data;
  public:
    EDtrAddr(Register r, EDtrOff off) : data(RN(r) | off.encode()) {}
    uint32 encode() { return data; }
};

class VFPOff
{
    uint32 data;
  protected:
    VFPOff(datastore::Imm8VFPOffData imm, IsUp_ isup)
        : data(imm.encode() | (uint32)isup) {}
  public:
    uint32 encode() { return data; }
};

class VFPOffImm : public VFPOff
{
  public:
    VFPOffImm(uint32 imm)
        : VFPOff(datastore::Imm8VFPOffData(imm >> 2), imm < 0 ? IsDown : IsUp) {}
};
class VFPAddr
{
    uint32 data;
    friend class Operand;
    VFPAddr(uint32 blob) : data(blob) {}
  public:
    VFPAddr(Register base, VFPOff off)
        : data(RN(base) | off.encode())
    {
    }
    uint32 encode() { return data; }
};

class VFPImm {
    uint32 data;
  public:
    VFPImm(uint32 top);
    uint32 encode() { return data; }
    bool isValid() { return data != -1U; }
};

class BOffImm
{
    uint32 data;
  public:
    uint32 encode() {
        return data;
    }
    BOffImm(int offset) : data (offset >> 2 & 0x00ffffff) {
        JS_ASSERT ((offset & 0x3) == 0);
        JS_ASSERT (offset >= -33554432);
        JS_ASSERT (offset <= 33554428);
    }
};
class Imm16
{
    uint32 lower : 12;
    uint32 pad : 4;
    uint32 upper : 4;
  public:
    Imm16(uint32 imm)
        : lower(imm & 0xfff), pad(0), upper((imm>>12) & 0xf)
    {
        JS_ASSERT(uint32(lower | (upper << 12)) == imm);
    }
    uint32 encode() { return lower | upper << 16; }
};




class FloatOp
{
    uint32 data;
};





class Operand
{
    
    
    
  public:

    enum Tag_ {
        OP2,
        DTR,
        EDTR,
        VDTR,
        FOP
    };
  private:
    Tag_ Tag;
    uint32 data;
  public:
    Operand (Operand2 init) : Tag(OP2), data(init.encode()) {}
    Operand (Register reg)  : Tag(OP2), data(O2Reg(reg).encode()) {}
    Operand (FloatRegister reg)  : Tag(FOP), data(reg.code()) {}
    Operand (DTRAddr addr) : Tag(DTR), data(addr.encode()) {}
    Operand (VFPAddr addr) : Tag(VDTR), data(addr.encode()) {}
    Tag_ getTag() { return Tag; }
    Operand2 toOp2() { return Operand2(data); }
    DTRAddr toDTRAddr() {JS_ASSERT(Tag == DTR); return DTRAddr(data); }
    VFPAddr toVFPAddr() {JS_ASSERT(Tag == VDTR); return VFPAddr(data); }
};

class Assembler
{
  public:
    
    typedef enum {
        EQ = 0x00000000, 
        NE = 0x10000000, 
        CS = 0x20000000,
        CC = 0x30000000,
        MI = 0x40000000,
        PL = 0x50000000,
        VS = 0x60000000,
        VC = 0x70000000,
        HI = 0x80000000,
        LS = 0x90000000,
        GE = 0xa0000000,
        LT = 0xb0000000,
        GT = 0xc0000000,
        LE = 0xd0000000,
        AL = 0xe0000000
    } ARMCondition;

    enum Condition {
        Equal = EQ,
        NotEqual = NE,
        Above = HI,
        AboveOrEqual = CS,
        Below = CC,
        BelowOrEqual = LS,
        GreaterThan = GT,
        GreaterThanOrEqual = GE,
        LessThan = LT,
        LessThanOrEqual = LE,
        Overflow = VS,
        Signed = MI,
        Zero = EQ,
        NonZero = NE,
        Always  = AL,

        NotEqual_Unordered = NotEqual,
        GreaterEqual_Unordered = AboveOrEqual,
        Unordered = Overflow,
        NotUnordered = VC,
        Greater_Unordered = Above
        
    };
    Condition getCondition(uint32 inst) {
        return (Condition) (0xf0000000 & inst);
    }
  protected:

    class BufferOffset;
    BufferOffset nextOffset () {
        return BufferOffset(m_buffer.uncheckedSize());
    }
    BufferOffset labelOffset (Label *l) {
        return BufferOffset(l->bound());
    }

    uint32 * editSrc (BufferOffset bo) {
        return (uint32*)(((char*)m_buffer.data()) + bo.getOffset());
    }
    
    
    class BufferOffset
    {
        int offset;
      public:
        friend BufferOffset nextOffset();
        explicit BufferOffset(int offset_) : offset(offset_) {}
        int getOffset() { return offset; }
        BOffImm diffB(BufferOffset other) {
            return BOffImm(offset - other.offset-8);
        }
        BOffImm diffB(Label *other) {
            JS_ASSERT(other->bound());
            return BOffImm(offset - other->offset()-8);
        }
        explicit BufferOffset(Label *l) : offset(l->offset()) {
        }
        BufferOffset() : offset(INT_MIN) {}
        bool assigned() { return offset != INT_MIN; };
    };

    
    
    struct RelativePatch {
        
        
        BufferOffset offset;
        void *target;
        Relocation::Kind kind;

        RelativePatch(BufferOffset offset, void *target, Relocation::Kind kind)
          : offset(offset),
            target(target),
            kind(kind)
        { }
    };

    js::Vector<DeferredData *, 0, SystemAllocPolicy> data_;
    js::Vector<CodeLabel *, 0, SystemAllocPolicy> codeLabels_;
    js::Vector<RelativePatch, 8, SystemAllocPolicy> jumps_;
    CompactBufferWriter jumpRelocations_;
    CompactBufferWriter dataRelocations_;
    CompactBufferWriter relocations_;
    size_t dataBytesNeeded_;

    bool enoughMemory_;

    typedef JSC::AssemblerBufferWithConstantPool<2048, 4, 4, js::ion::Assembler> ARMBuffer;
    ARMBuffer m_buffer;



public:
    Assembler()
      : dataBytesNeeded_(0),
        enoughMemory_(true),
        dtmActive(false),
        dtmCond(Always)

    {
    }
    static Condition InvertCondition(Condition cond);

    
    void trace(JSTracer *trc);

    bool oom() const;

    void executableCopy(void *buffer);
    void processDeferredData(IonCode *code, uint8 *data);
    void processCodeLabels(IonCode *code);
    void copyJumpRelocationTable(uint8 *buffer);
    void copyDataRelocationTable(uint8 *buffer);

    bool addDeferredData(DeferredData *data, size_t bytes);

    bool addCodeLabel(CodeLabel *label);

    
    size_t size() const;
    
    size_t jumpRelocationTableBytes() const;
    size_t dataRelocationTableBytes() const;
    
    size_t dataSize() const;
    size_t bytesNeeded() const;
    
    void writeInst(uint32 x);

  public:
    void align(int alignment);
    void as_alu(Register dest, Register src1, Operand2 op2,
                ALUOp op, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_mov(Register dest,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_mvn(Register dest, Operand2 op2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    
    void as_and(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_bic(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_eor(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_orr(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    
    void as_adc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_add(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_sbc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_sub(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_rsb(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    void as_rsc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    
    void as_cmn(Register src1, Operand2 op2,
                Condition c = Always);
    void as_cmp(Register src1, Operand2 op2,
                Condition c = Always);
    void as_teq(Register src1, Operand2 op2,
                Condition c = Always);
    void as_tst(Register src1, Operand2 op2,
                Condition c = Always);

    
    
    
    void as_movw(Register dest, Imm16 imm, Condition c = Always);
    void as_movt(Register dest, Imm16 imm, Condition c = Always);
    
    
    
    void as_dtr(LoadStore ls, int size, Index mode,
                Register rt, DTRAddr addr, Condition c = Always);
    
    
    
    void as_extdtr(LoadStore ls, int size, bool IsSigned, Index mode,
                   Register rt, EDtrAddr addr, Condition c = Always);

    void as_dtm(LoadStore ls, Register rn, uint32 mask,
                DTMMode mode, DTMWriteBack wb, Condition c = Always);

    

    
    
    void as_bx(Register r, Condition c = Always);

    
    
    
    void as_b(BOffImm off, Condition c);

    void as_b(Label *l, Condition c = Always);
    void as_b(BOffImm off, Condition c, BufferOffset inst);

    
    
    
    
    void as_blx(Label *l);

    void as_blx(Register r, Condition c = Always);
    void as_bl(BOffImm off, Condition c);
    
    
    void as_bl();
    
    
    void as_bl(Label *l, Condition c);
    void as_bl(BOffImm off, Condition c, BufferOffset inst);

    
  private:
    enum vfp_size {
        isDouble = 1 << 8,
        isSingle = 0 << 8
    };
    void writeVFPInst(vfp_size sz, uint32 blob);
    
    
    void as_vfp_float(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                      VFPOp op, Condition c = Always);

  public:
    void as_vadd(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    void as_vdiv(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    void as_vmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    void as_vnmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always);

    void as_vnmla(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always);

    void as_vnmls(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always);

    void as_vneg(VFPRegister vd, VFPRegister vm, Condition c = Always);

    void as_vsqrt(VFPRegister vd, VFPRegister vm, Condition c = Always);

    void as_vabs(VFPRegister vd, VFPRegister vm, Condition c = Always);

    void as_vsub(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    void as_vcmp(VFPRegister vd, VFPRegister vm,
                 Condition c = Always);
    void as_vcmpz(VFPRegister vd,  Condition c = Always);

    
    void as_vmov(VFPRegister vd, VFPRegister vsrc, Condition c = Always);
    
    enum FloatToCore_ {
        FloatToCore = 1 << 20,
        CoreToFloat = 0 << 20
    };
  private:
    enum VFPXferSize {
        WordTransfer   = 0x02000010,
        DoubleTransfer = 0x00400010
    };
  public:
    
    
    
    
    

    void as_vxfer(Register vt1, Register vt2, VFPRegister vm, FloatToCore_ f2c,
                  Condition c = Always);

    
    
    void as_vcvt(VFPRegister vd, VFPRegister vm,
                 Condition c = Always);
    
    void as_vdtr(LoadStore ls, VFPRegister vd, VFPAddr addr,
                 Condition c = Always );

    
    

    void as_vdtm(LoadStore st, Register rn, VFPRegister vd, int length,
                 Condition c = Always);

    void as_vimm(VFPRegister vd, VFPImm imm, Condition c = Always);

    void as_vmrs(Register r, Condition c = Always);
    
    bool nextLink(BufferOffset b, BufferOffset *next);
    void bind(Label *label, BufferOffset boff = BufferOffset());
    static void Bind(IonCode *code, AbsoluteLabel *label, const void *address);
    void retarget(Label *label, Label *target);
    
    void retarget(Label *label, void *target, Relocation::Kind reloc);

    void call(Label *label);

    void as_bkpt();

  public:
    static void TraceJumpRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);
    static void TraceDataRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);

    
    
    void flush() { }

    
    
    void executableCopy(uint8 *buffer);

    
    
    void cvttsd2s(const FloatRegister &src, const Register &dest) {
    }

    void as_b(void *target, Relocation::Kind reloc) {
        JS_NOT_REACHED("feature NYI");
#if 0
        JmpSrc src = masm.branch();
        addPendingJump(src, target, reloc);
#endif
    }
    void as_b(Condition cond, void *target, Relocation::Kind reloc) {
        JS_NOT_REACHED("feature NYI");
#if 0
        JmpSrc src = masm.branch(cond);
        addPendingJump(src, target, reloc);
#endif
    }
#if 0
    void movsd(const double *dp, const FloatRegister &dest) {
    }
    void movsd(AbsoluteLabel *label, const FloatRegister &dest) {
        JS_ASSERT(!label->bound());
        
        
    }
#endif
    
    
    void startDataTransferM(LoadStore ls, Register rm,
                            DTMMode mode, DTMWriteBack update = NoWriteBack,
                            Condition c = Always)
    {
        JS_ASSERT(!dtmActive);
        dtmUpdate = update;
        dtmBase = rm;
        dtmLoadStore = ls;
        dtmLastReg = -1;
        dtmRegBitField = 0;
        dtmActive = 1;
        dtmCond = c;
        dtmMode = mode;
    }

    void transferReg(Register rn) {
        JS_ASSERT(dtmActive);
        JS_ASSERT(rn.code() > dtmLastReg);
        dtmRegBitField |= 1 << rn.code();
        if (dtmLoadStore == IsLoad && rn.code() == 13 && dtmBase.code() == 13) {
            JS_ASSERT("ARM Spec says this is invalid");
        }
    }
    void finishDataTransfer() {
        dtmActive = false;
        as_dtm(dtmLoadStore, dtmBase, dtmRegBitField, dtmMode, dtmUpdate, dtmCond);
    }

    void startFloatTransferM(LoadStore ls, Register rm,
                             DTMMode mode, DTMWriteBack update = NoWriteBack,
                             Condition c = Always)
    {
        JS_ASSERT(!dtmActive);
        dtmActive = true;
        dtmUpdate = update;
        dtmLoadStore = ls;
        dtmBase = rm;
        dtmCond = c;
        dtmLastReg = -1;
    }
    void transferFloatReg(VFPRegister rn)
    {
        if (dtmLastReg == -1) {
            vdtmFirstReg = rn;
        } else {
            JS_ASSERT(dtmLastReg > 0);
            JS_ASSERT(rn.code() == unsigned(dtmLastReg) + 1);
        }
        dtmLastReg = rn.code();
    }
    void finishFloatTransfer() {
        JS_ASSERT(dtmActive);
        dtmActive = false;
        JS_ASSERT(dtmLastReg != -1);
        
        int len = dtmLastReg - vdtmFirstReg.code() + 1;
        as_vdtm(dtmLoadStore, dtmBase, vdtmFirstReg, len, dtmCond);
    }
private:
    int dtmRegBitField;
    int dtmLastReg;
    Register dtmBase;
    VFPRegister vdtmFirstReg;
    DTMWriteBack dtmUpdate;
    DTMMode dtmMode;
    LoadStore dtmLoadStore;
    bool dtmActive;
    Condition dtmCond;
  public:

    enum {
        padForAlign8  = (int)0x00,
        padForAlign16 = (int)0x0000,
        padForAlign32 = (int)0xe12fff7f  
    };
    static uint32 patchConstantPoolLoad(uint32 load, uint32 value)
    {
        value = (value << 1) + 1;
        ASSERT(!(value & ~0xfff));
        return (load & ~0xfff) | value;
    }

    static void patchConstantPoolLoad(void* loadAddr, void* constPoolAddr) {
        JS_NOT_REACHED("ARMAssembler holdover");
    }


    static uint32 placeConstantPoolBarrier(int offset)
    {
        JS_NOT_REACHED("ARMAssembler holdover");
#if 0
        offset = (offset - sizeof(ARMWord)) >> 2;
        ASSERT((offset <= BOFFSET_MAX && offset >= BOFFSET_MIN));
        return AL | B | (offset & BRANCH_MASK);
#endif
        return -1;
    }

}; 

static const uint32 NumArgRegs = 4;

static inline bool
GetArgReg(uint32 arg, Register *out)
{
    switch (arg) {
      case 0:
        *out = r0;
        return true;
      case 1:
        *out = r1;
        return true;
      case 2:
        *out = r2;
        return true;
      case 3:
        *out = r3;
        return true;
      default:
        return false;
    }
}

static inline uint32
GetArgStackDisp(uint32 arg)
{
    JS_ASSERT(arg >= NumArgRegs);
    return (arg - NumArgRegs) * STACK_SLOT_SIZE;
}

class DoubleEncoder {
    uint32 rep(bool b, uint32 count) {
        uint ret = 0;
        for (uint32 i = 0; i < count; i++)
            ret = (ret << 1) | b;
        return ret;
    }
    uint32 encode(uint8 value) {
        
        
        
        
        bool a = value >> 7;
        bool b = value >> 6 & 1;
        bool B = !b;
        uint32 cdefgh = value & 0x3f;
        return a << 31 |
            B << 30 |
            rep(b, 8) << 22 |
            cdefgh << 16;
    }
    struct DoubleEntry {
        uint32 dblTop;
        datastore::Imm8VFPImmData data;
        DoubleEntry(uint32 dblTop_, datastore::Imm8VFPImmData data_)
            : dblTop(dblTop_), data(data_) {}
        DoubleEntry() :dblTop(-1) {}
    };
    DoubleEntry table [256];
    
    static DoubleEncoder _this;
    DoubleEncoder()
    {
        for (int i = 0; i < 256; i++) {
            table[i] = DoubleEntry(encode(i), datastore::Imm8VFPImmData(i));
        }
    }

  public:
    static bool lookup(uint32 top, datastore::Imm8VFPImmData *ret) {
        for (int i = 0; i < 256; i++) {
            if (_this.table[i].dblTop == top) {
                *ret = _this.table[i].data;
                return true;
            }
        }
        return false;
    }
};

} 
} 

#endif 
