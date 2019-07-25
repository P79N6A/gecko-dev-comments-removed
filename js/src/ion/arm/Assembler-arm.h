








































#ifndef jsion_cpu_arm_assembler_h__
#define jsion_cpu_arm_assembler_h__

#include "ion/shared/Assembler-shared.h"
#include "assembler/assembler/ARMAssembler.h"
#include "ion/CompactBuffer.h"
#include "ion/IonCode.h"

namespace js {
namespace ion {







static const Register r0  = { JSC::ARMRegisters::r0 };
static const Register r1  = { JSC::ARMRegisters::r1 };
static const Register r2  = { JSC::ARMRegisters::r2 };
static const Register r3  = { JSC::ARMRegisters::r3 };
static const Register r4  = { JSC::ARMRegisters::r4 };
static const Register r5  = { JSC::ARMRegisters::r5 };
static const Register r6  = { JSC::ARMRegisters::r6 };
static const Register r7  = { JSC::ARMRegisters::r7 };
static const Register r8  = { JSC::ARMRegisters::r8 };
static const Register r9  = { JSC::ARMRegisters::r9 };
static const Register r10 = { JSC::ARMRegisters::r10 };
static const Register r11 = { JSC::ARMRegisters::r11 };
static const Register r12 = { JSC::ARMRegisters::ip };
static const Register ip  = { JSC::ARMRegisters::ip };
static const Register sp  = { JSC::ARMRegisters::sp };
static const Register r14 = { JSC::ARMRegisters::lr };
static const Register lr  = { JSC::ARMRegisters::lr };
static const Register pc  = { JSC::ARMRegisters::pc };

static const Register ScratchRegister = {JSC::ARMRegisters::ip};

static const Register InvalidReg = { JSC::ARMRegisters::invalid_reg };
static const FloatRegister InvalidFloatReg = { JSC::ARMRegisters::invalid_freg };

static const Register JSReturnReg_Type = r3;
static const Register JSReturnReg_Data = r2;
static const Register StackPointer = sp;
static const Register ReturnReg = r0;
static const FloatRegister ScratchFloatReg = { JSC::ARMRegisters::d0 };

static const FloatRegister d0 = {JSC::ARMRegisters::d0};
static const FloatRegister d1 = {JSC::ARMRegisters::d1};
static const FloatRegister d2 = {JSC::ARMRegisters::d2};
static const FloatRegister d3 = {JSC::ARMRegisters::d3};
static const FloatRegister d4 = {JSC::ARMRegisters::d4};
static const FloatRegister d5 = {JSC::ARMRegisters::d5};
static const FloatRegister d6 = {JSC::ARMRegisters::d6};
static const FloatRegister d7 = {JSC::ARMRegisters::d7};
static const FloatRegister d8 = {JSC::ARMRegisters::d8};
static const FloatRegister d9 = {JSC::ARMRegisters::d9};
static const FloatRegister d10 = {JSC::ARMRegisters::d10};
static const FloatRegister d11 = {JSC::ARMRegisters::d11};
static const FloatRegister d12 = {JSC::ARMRegisters::d12};
static const FloatRegister d13 = {JSC::ARMRegisters::d13};
static const FloatRegister d14 = {JSC::ARMRegisters::d14};
static const FloatRegister d15 = {JSC::ARMRegisters::d15};

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
        : _isMissing(b), _isInvalid(false) {}
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
    VFPRegister doubleOverlay() {
        JS_ASSERT(!_isInvalid);
        if (kind != Double) {
            return VFPRegister(_code >> 1, Double);
        } else {
            return *this;
        }
    }
    VFPRegister singleOverlay() {
        JS_ASSERT(!_isInvalid);
        if (kind == Double) {
            
            ASSERT(_code < 16);
            return VFPRegister(_code << 1, Double);
        } else {
            return VFPRegister(_code, Single);
        }
    }
    VFPRegister intOverlay() {
        JS_ASSERT(!_isInvalid);
        if (kind == Double) {
            
            ASSERT(_code < 16);
            return VFPRegister(_code << 1, Double);
        } else {
            return VFPRegister(_code, Int);
        }
    }
    bool isInvalid() {
        return _isInvalid;
    }
    bool isMissing() {
        JS_ASSERT(!_isInvalid);
        return _isMissing;
    }
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
    opv_cmp  = 0xB << 20 | 0x1 << 6 | 0x4 << 16
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
    uint32 toInt() {
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
    uint32 toInt() {
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
    uint32 toInt() {
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
    uint32 toInt() {
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
    Imm8VFPImmData() : imm4L(-1), imm4H(-1), isInvalid(-1) {}
    Imm8VFPImmData(uint32 imm) : imm4L(imm&0xf), imm4H(imm>>4), isInvalid(0) {
        JS_ASSERT(imm <= 0xff);
    }
};

struct Imm12Data
{
    uint32 data : 12;
    Imm12Data(uint32 imm) : data(imm) { JS_ASSERT(data == imm); }
    uint32 toInt() { return data; }
};

struct RIS
{
    uint32 ShiftAmount : 5;
    RIS(uint32 imm) : ShiftAmount(imm) { ASSERT(ShiftAmount == imm); }
    uint32 toInt () {
        return ShiftAmount;
    }
};
struct RRS
{
    uint32 MustZero : 1;
    
    uint32 RS : 4;
    RRS(uint32 rs) : RS(rs) { ASSERT(rs == RS); }
    uint32 toInt () {return RS << 1;}
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
        : oper(base.invalid ? -1 : (base.toInt() | (uint32)IsImmOp2)),
          invalid(base.invalid)
    {
    }
    Operand2(datastore::Reg base) : oper(base.toInt() | (uint32)IsNotImmOp2) {}
  private:
    friend class Operand;
    Operand2(int blob) : oper(blob) {}
  public:
    uint32 toInt() { return oper; }
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
        : Operand2(datastore::Reg(rm.code(), type, 0, shiftImm.toInt())) {}
    Op2Reg(Register rm, ShiftType type, datastore::RRS shiftReg)
        : Operand2(datastore::Reg(rm.code(), type, 1, shiftReg.toInt())) {}
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
    : data(immdata.toInt() | (uint32)IsImmDTR | ((uint32)iu)) {}
    DtrOff(datastore::Reg reg, IsUp_ iu = IsUp)
        : data(reg.toInt() | (uint32) IsNotImmDTR | iu) {}
  public:
    uint32 toInt() { return data; }
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
        : DtrOff(datastore::Reg(rn.code(), type, 0, shiftImm.toInt())) {}
    DtrOffReg(Register rn, ShiftType type, datastore::RRS shiftReg)
        : DtrOff(datastore::Reg(rn.code(), type, 1, shiftReg.toInt())) {}
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
        : data(dtr.toInt() | (reg.code() << 16)) {}
    uint32 toInt() { return data; }
  private:
    friend class Operand;
    DTRAddr(uint32 blob) : data(blob) {}
};



class EDtrOff
{
  protected:
    uint32 data;
    EDtrOff(datastore::Imm8Data imm8, IsUp_ iu = IsUp)
        : data(imm8.toInt() | IsImmEDTR | (uint32)iu) {}
    EDtrOff(Register rm, IsUp_ iu = IsUp)
        : data(rm.code() | IsNotImmEDTR | iu) {}
  public:
    uint32 toInt() { return data; }
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
    EDtrAddr(Register r, EDtrOff off) : data(RN(r) | off.toInt()) {}
    uint32 toInt() { return data; }
};

class VFPOff
{
    uint32 data;
  protected:
    VFPOff(datastore::Imm8VFPOffData imm, IsUp_ isup)
        : data(imm.toInt() | (uint32)isup) {}
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
    uint32 toInt() { return data; }
};

class VFPImm {
    uint32 data;
  public:
    VFPImm(uint32 top);
    uint32 encode() { return data; }
    bool isValid() { return data != -1; }
};

class BOffImm
{
    uint32 data;
  public:
    uint32 toInt() {
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
    uint32 toInt() { return lower | upper << 16; }
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
    Operand (Operand2 init) : Tag(OP2), data(init.toInt()) {}
    Operand (Register reg)  : Tag(OP2), data(O2Reg(reg).toInt()) {}
    Operand (FloatRegister reg)  : Tag(FOP), data(reg.code()) {}
    Operand (DTRAddr addr) : Tag(DTR), data(addr.toInt()) {}
    Operand (VFPAddr addr) : Tag(VDTR), data(addr.toInt()) {}
    Tag_ getTag() { return Tag; }
    Operand2 toOp2() { return Operand2(data); }
    DTRAddr toDTRAddr() {JS_ASSERT(Tag == DTR); return DTRAddr(data); }
    VFPAddr toVFPAddr() {JS_ASSERT(Tag == VDTR); return VFPAddr(data); }
};

class Assembler
{
  public:
    enum Condition {
        Equal = JSC::ARMAssembler::EQ,
        NotEqual = JSC::ARMAssembler::NE,
        Above = JSC::ARMAssembler::HI,
        AboveOrEqual = JSC::ARMAssembler::CS,
        Below = JSC::ARMAssembler::CC,
        BelowOrEqual = JSC::ARMAssembler::LS,
        GreaterThan = JSC::ARMAssembler::GT,
        GreaterThanOrEqual = JSC::ARMAssembler::GE,
        LessThan = JSC::ARMAssembler::LT,
        LessThanOrEqual = JSC::ARMAssembler::LE,
        Overflow = JSC::ARMAssembler::VS,
        Signed = JSC::ARMAssembler::MI,
        Zero = JSC::ARMAssembler::EQ,
        NonZero = JSC::ARMAssembler::NE,
        Always  = JSC::ARMAssembler::AL,

        NotEqual_Unordered = NotEqual,
        GreaterEqual_Unordered = AboveOrEqual,
        Unordered = Overflow,
        NotUnordered = JSC::ARMAssembler::VC,
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

    typedef JSC::AssemblerBufferWithConstantPool<2048, 4, 4, JSC::ARMAssembler> ARMBuffer;
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

    bool oom() const {
        return m_buffer.oom() ||
            !enoughMemory_ ||
            jumpRelocations_.oom();
    }

    void executableCopy(void *buffer);
    void processDeferredData(IonCode *code, uint8 *data);
    void processCodeLabels(IonCode *code);
    void copyJumpRelocationTable(uint8 *buffer);
    void copyDataRelocationTable(uint8 *buffer);

    bool addDeferredData(DeferredData *data, size_t bytes) {
        data->setOffset(dataBytesNeeded_);
        dataBytesNeeded_ += bytes;
        if (dataBytesNeeded_ >= MAX_BUFFER_SIZE)
            return false;
        return data_.append(data);
    }

    bool addCodeLabel(CodeLabel *label) {
        return codeLabels_.append(label);
    }

    
    size_t size() const {
        return m_buffer.uncheckedSize();
    }
    
    size_t jumpRelocationTableBytes() const {
        return jumpRelocations_.length();
    }
    size_t dataRelocationTableBytes() const {
        return dataRelocations_.length();
    }
    
    size_t dataSize() const {
        return dataBytesNeeded_;
    }
    size_t bytesNeeded() const {
        return size() +
               dataSize() +
               jumpRelocationTableBytes() +
               dataRelocationTableBytes();
    }
    
    void writeBlob(uint32 x)
    {
        m_buffer.putInt(x);
    }

  public:
    void align(int alignment) {
        while (!m_buffer.isAligned(alignment))
            as_mov(r0, O2Reg(r0));

    }
    void as_alu(Register dest, Register src1, Operand2 op2,
                ALUOp op, SetCond_ sc = NoSetCond, Condition c = Always) {
        writeBlob((int)op | (int)sc | (int) c | op2.toInt() |
                  ((dest == InvalidReg) ? 0 : RD(dest)) |
                  ((src1 == InvalidReg) ? 0 : RN(src1)));
    }
    void as_mov(Register dest,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, InvalidReg, op2, op_mov, sc, c);
    }
    void as_mvn(Register dest, Operand2 op2,
                SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, InvalidReg, op2, op_mvn, sc, c);
    }
    
    void as_and(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_and, sc, c);
    }
    void as_bic(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_bic, sc, c);
    }
    void as_eor(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_eor, sc, c);
    }
    void as_orr(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_orr, sc, c);
    }
    
    void as_adc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_adc, sc, c);
    }
    void as_add(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_add, sc, c);
    }
    void as_sbc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_sbc, sc, c);
    }
    void as_sub(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_sub, sc, c);
    }
    void as_rsb(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_rsb, sc, c);
    }
    void as_rsc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always) {
        as_alu(dest, src1, op2, op_rsc, sc, c);
    }
    
    void as_cmn(Register src1, Operand2 op2,
                Condition c = Always) {
        as_alu(InvalidReg, src1, op2, op_cmn, SetCond, c);
    }
    void as_cmp(Register src1, Operand2 op2,
                Condition c = Always) {
        as_alu(InvalidReg, src1, op2, op_cmp, SetCond, c);
    }
    void as_teq(Register src1, Operand2 op2,
                Condition c = Always) {
        as_alu(InvalidReg, src1, op2, op_teq, SetCond, c);
    }
    void as_tst(Register src1, Operand2 op2,
                Condition c = Always) {
        as_alu(InvalidReg, src1, op2, op_tst, SetCond, c);
    }

    
    
    
    void as_movw(Register dest, Imm16 imm, Condition c = Always) {
        JS_ASSERT(hasMOVWT());
        writeBlob(0x03000000 | c | imm.toInt() | RD(dest));
    }
    void as_movt(Register dest, Imm16 imm, Condition c = Always) {
        JS_ASSERT(hasMOVWT());
        writeBlob(0x03400000 | c | imm.toInt() | RD(dest));
    }
    
    
    
    void as_dtr(LoadStore ls, int size, Index mode,
                Register rt, DTRAddr addr, Condition c = Always)
    {
        JS_ASSERT(size == 32 || size == 8);
        writeBlob( 0x04000000 | ls | (size == 8 ? 0x00400000 : 0) | mode | c |
                   RT(rt) | addr.toInt());
        return;
    }
    
    
    
    void as_extdtr(LoadStore ls, int size, bool IsSigned, Index mode,
                   Register rt, EDtrAddr addr, Condition c = Always)
    {
        int extra_bits2 = 0;
        int extra_bits1 = 0;
        switch(size) {
          case 8:
            JS_ASSERT(IsSigned);
            JS_ASSERT(ls!=IsStore);
            break;
          case 16:
            
            
            extra_bits2 = 0x01;
            extra_bits1 = (ls == IsStore) ? 0 : 1;
            if (IsSigned) {
                JS_ASSERT(ls != IsStore);
                extra_bits2 |= 0x2;
            }
            break;
          case 64:
            if (ls == IsStore) {
                extra_bits2 = 0x3;
            } else {
                extra_bits2 = 0x2;
            }
            extra_bits1 = 0;
            break;
          default:
            JS_NOT_REACHED("SAY WHAT?");
        }
        writeBlob(extra_bits2 << 5 | extra_bits1 << 20 | 0x90 |
                  addr.toInt() | RT(rt) | c);
        return;
    }

    void as_dtm(LoadStore ls, Register rn, uint32 mask,
                DTMMode mode, DTMWriteBack wb, Condition c = Always)
    {
        writeBlob(0x08000000 | RN(rn) | ls |
                  mode | mask | c | wb);

        return;
    }

    

    
    
    void as_bx(Register r, Condition c = Always)
    {
        writeBlob(((int) c) | op_bx | r.code());
    }

    
    
    
    void as_b(BOffImm off, Condition c)
    {
        writeBlob(((int)c) | op_b | off.toInt());
    }

    void as_b(Label *l, Condition c = Always)
    {
        BufferOffset next = nextOffset();
        if (l->bound()) {
            as_b(BufferOffset(l).diffB(next), c);
        } else {
            
            int32 old = l->use(next.getOffset());
            if (old == LabelBase::INVALID_OFFSET) {
                old = -4;
            }
            
            
            as_b(BOffImm(old), c);
        }
    }
    void as_b(BOffImm off, Condition c, BufferOffset inst)
    {
        *editSrc(inst) = ((int)c) | op_b | off.toInt();
    }

    
    
    
    
    void as_blx(Label *l)
    {
        JS_NOT_REACHED("Feature NYI");
    }

    void as_blx(Register r, Condition c = Always)
    {
        writeBlob(((int) c) | op_blx | r.code());
    }
    void as_bl(BOffImm off, Condition c)
    {
        writeBlob(((int)c) | op_bl | off.toInt());
    }
    
    
    void as_bl()
    {
        JS_NOT_REACHED("Feature NYI");
    }
    
    
    void as_bl(Label *l, Condition c)
    {
        BufferOffset next = nextOffset();
        if (l->bound()) {
            as_bl(BufferOffset(l).diffB(next), c);
        } else {
            int32 old = l->use(next.getOffset());
            
            if (old == -1) {
                old = -4;
            }
            
            
            as_bl(BOffImm(old), c);
        }
    }
    void as_bl(BOffImm off, Condition c, BufferOffset inst)
    {
        *editSrc(inst) = ((int)c) | op_bl | off.toInt();
    }

    
    enum vfp_size {
        isDouble = 1 << 8,
        isSingle = 0 << 8
    };
    
    
    void as_vfp_float(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                      VFPOp op, Condition c = Always)
    {
        
        JS_ASSERT(vd.equiv(vn) && vd.equiv(vm));
        vfp_size sz = isDouble;
        if (!vd.isDouble()) {
            sz = isSingle;
        }
        writeBlob(VD(vd) | VN(vn) | VM(vm) | op | c | sz | 0x0e000a00);
    }

    void as_vadd(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always)
    {
        as_vfp_float(vd, vn, vm, opv_add, c);
    }

    void as_vdiv(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always)
    {
        as_vfp_float(vd, vn, vm, opv_mul, c);
    }

    void as_vmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always)
    {
        as_vfp_float(vd, vn, vm, opv_mul, c);
    }

    void as_vnmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always)
    {
        as_vfp_float(vd, vn, vm, opv_mul, c);
        JS_NOT_REACHED("Feature NYI");
    }

    void as_vnmla(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always)
    {
        JS_NOT_REACHED("Feature NYI");
    }

    void as_vnmls(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always)
    {
        JS_NOT_REACHED("Feature NYI");
    }

    void as_vneg(VFPRegister vd, VFPRegister vm, Condition c = Always)
    {
        as_vfp_float(vd, NoVFPRegister, vm, opv_neg, c);
    }

    void as_vsqrt(VFPRegister vd, VFPRegister vm, Condition c = Always)
    {
        as_vfp_float(vd, NoVFPRegister, vm, opv_sqrt, c);
    }

    void as_vabs(VFPRegister vd, VFPRegister vm, Condition c = Always)
    {
        as_vfp_float(vd, NoVFPRegister, vm, opv_abs, c);
    }

    void as_vsub(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always)
    {
        as_vfp_float(vd, vn, vm, opv_sub, c);
    }

    void as_vcmp(VFPRegister vd, VFPRegister vm,
                 Condition c = Always)
    {
        as_vfp_float(vd, NoVFPRegister, vm, opv_sub, c);
    }

    
    void as_vmov(VFPRegister vd, VFPRegister vsrc, Condition c = Always)
    {
        as_vfp_float(vd, NoVFPRegister, vsrc, opv_mov, c);
    }
    
    enum FloatToCore_ {
        FloatToCore = 1 << 20,
        CoreToFloat = 0 << 20
    };

    enum VFPXferSize {
        WordTransfer   = 0x0E000A10,
        DoubleTransfer = 0x0C400A10
    };

    
    
    
    
    

    void as_vxfer(Register vt1, Register vt2, VFPRegister vm, FloatToCore_ f2c,
                  Condition c = Always)
    {
        vfp_size sz = isSingle;
        if (vm.isDouble()) {
            
            
            
            
            
            
            JS_ASSERT(vt2 != InvalidReg);
            sz = isDouble;
        }
        VFPXferSize xfersz = WordTransfer;
        if (vt2 != InvalidReg) {
            
            xfersz = DoubleTransfer;
        }
        writeBlob(xfersz | f2c | c | sz |
                 RT(vt1) | ((vt2 != InvalidReg) ? RN(vt2) : 0) | VM(vm));
    }

    
    
    void as_vcvt(VFPRegister vd, VFPRegister vm,
                 Condition c = Always)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    
    void as_vdtr(LoadStore ls, VFPRegister vd, VFPAddr addr,
                 Condition c = Always ) {
        vfp_size sz = isDouble;
        if (!vd.isDouble()) {
            sz = isSingle;
        }

        writeBlob(0x0D000A00 | addr.toInt() | VD(vd) | sz | c);
    }

    
    

    void as_vdtm(LoadStore st, Register rn, VFPRegister vd, int length,
                 Condition c = Always)
    {
        JS_ASSERT(length <= 16 && length >= 0);
        vfp_size sz = isDouble;
        if (!vd.isDouble()) {
            sz = isSingle;
        } else {
            length *= 2;
        }
        writeBlob(dtmLoadStore | RN(rn) | VD(vd) |
                  length |
                  dtmMode | dtmUpdate | dtmCond |
                  0x0C000B00 | sz);
    }

    void as_vimm(VFPRegister vd, VFPImm imm, Condition c = Always)
    {
        vfp_size sz = isDouble;
        if (!vd.isDouble()) {
            
            sz = isSingle;
            JS_NOT_REACHED("non-double immediate");
        }
        writeBlob(c | sz | imm.encode() | VD(vd) | 0x0EB00A00);

    }

    bool nextLink(BufferOffset b, BufferOffset *next)
    {
        uint32 branch = *editSrc(b);
        JS_ASSERT(((branch & op_b_mask) == op_b) ||
                  ((branch & op_b_mask) == op_bl));
        uint32 dest = (branch & op_b_dest_mask);
        
        if (dest == op_b_dest_mask)
            return false;
        
        dest = dest << 2;
        
        new (next) BufferOffset(dest);
        return true;
    }

    void bind(Label *label) {
        
        if (label->used()) {
            bool more;
            BufferOffset dest = nextOffset();
            BufferOffset b(label);
            do {
                BufferOffset next;
                more = nextLink(b, &next);
                uint32 branch = *editSrc(b);
                Condition c = getCondition(branch);
                switch (branch & op_b_mask) {
                  case op_b:
                    as_b(dest.diffB(b), c, b);
                    break;
                  case op_bl:
                    as_bl(dest.diffB(b), c, b);
                    break;
                  default:
                    JS_NOT_REACHED("crazy fixup!");
                }
                b = next;
            } while (more);
        }
        label->bind(nextOffset().getOffset());
    }

    static void Bind(IonCode *code, AbsoluteLabel *label, const void *address) {
#if 0
        uint8 *raw = code->raw();
        if (label->used()) {
            intptr_t src = label->offset();
            do {
                intptr_t next = reinterpret_cast<intptr_t>(JSC::ARMAssembler::getPointer(raw + src));
                JSC::ARMAssembler::setPointer(raw + src, address);
                src = next;
            } while (src != AbsoluteLabel::INVALID_OFFSET);
        }
        JS_ASSERT(((uint8 *)address - raw) >= 0 && ((uint8 *)address - raw) < INT_MAX);
        label->bind();
#endif
        JS_NOT_REACHED("Feature NYI");
    }

    void call(Label *label) {
#if 0
        if (label->bound()) {
            masm.linkJump(masm.call(), JmpDst(label->offset()));
        } else {
            JmpSrc j = masm.call();
            JmpSrc prev = JmpSrc(label->use(j.offset()));
            masm.setNextJump(j, prev);
        }
#endif
        JS_NOT_REACHED("Feature NYI");
    }

    void as_bkpt() {
        writeBlob(0xe1200070);
    }

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
            JS_ASSERT(rn.code() == dtmLastReg + 1);
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

class DoubleEncoder {
    uint32 rep(bool b, uint32 count) {
        uint ret = 0;
        for (int i = 0; i < count; i++)
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
