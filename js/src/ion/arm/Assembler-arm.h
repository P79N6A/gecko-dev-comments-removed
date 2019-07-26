








































#ifndef jsion_cpu_arm_assembler_h__
#define jsion_cpu_arm_assembler_h__

#include "ion/shared/Assembler-shared.h"
#include "assembler/assembler/AssemblerBufferWithConstantPool.h"
#include "ion/CompactBuffer.h"
#include "ion/IonCode.h"
#include "ion/arm/Architecture-arm.h"
#include "ion/shared/IonAssemblerBufferWithConstantPools.h"

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

static const Register OsrFrameReg = r3;
static const Register ArgumentsRectifierReg = r8;
static const Register CallTempReg0 = r5;
static const Register CallTempReg1 = r6;
static const Register CallTempReg2 = r7;
static const Register CallTempReg3 = r8;

static const Register PreBarrierReg = r1;

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






static const uint32 StackAlignment = 8;
static const bool StackKeptAligned = true;

static const Scale ScalePointer = TimesFour;

class Instruction;
class InstBranchImm;
uint32 RM(Register r);
uint32 RS(Register r);
uint32 RD(Register r);
uint32 RT(Register r);
uint32 RN(Register r);

uint32 maybeRD(Register r);
uint32 maybeRT(Register r);
uint32 maybeRN(Register r);

Register toRM (Instruction &i);
Register toRD (Instruction &i);
Register toR (Instruction &i);

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

  protected:
    RegType kind : 2;
    
    
    
    
    
    
    
    uint32 _code : 5;
    bool _isInvalid : 1;
    bool _isMissing : 1;

    VFPRegister(int  r, RegType k)
      : kind(k), _code (r), _isInvalid(false), _isMissing(false)
    { }

  public:
    VFPRegister()
      : _isInvalid(true), _isMissing(false)
    { }

    VFPRegister(bool b)
      : _isInvalid(false), _isMissing(b)
    { }

    VFPRegister(FloatRegister fr)
      : kind(Double), _code(fr.code()), _isInvalid(false), _isMissing(false)
    {
        JS_ASSERT(_code == (unsigned)fr.code());
    }

    VFPRegister(FloatRegister fr, RegType k)
      : kind(k), _code (fr.code()), _isInvalid(false), _isMissing(false)
    {
        JS_ASSERT(_code == (unsigned)fr.code());
    }
    bool isDouble() { return kind == Double; }
    bool isSingle() { return kind == Single; }
    bool isFloat() { return (kind == Double) || (kind == Single); }
    bool isInt() { return (kind == UInt) || (kind == Int); }
    bool isSInt()   { return kind == Int; }
    bool isUInt()   { return kind == UInt; }
    bool equiv(VFPRegister other) { return other.kind == kind; }
    size_t size() { return (kind == Double) ? 8 : 4; }
    bool isInvalid();
    bool isMissing();

    VFPRegister doubleOverlay();
    VFPRegister singleOverlay();
    VFPRegister sintOverlay();
    VFPRegister uintOverlay();

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

    uint32 code() const {
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


enum MULOp {
    opm_mul   = 0 << 21,
    opm_mla   = 1 << 21,
    opm_umaal = 2 << 21,
    opm_mls   = 3 << 21,
    opm_umull = 4 << 21,
    opm_umlal = 5 << 21,
    opm_smull = 6 << 21,
    opm_smlal = 7 << 21
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

ALUOp ALUNeg(ALUOp op, Register dest, Imm32 *imm, Register *negDest);
bool can_dbl(ALUOp op);
bool condsAreSafe(ALUOp op);


ALUOp getDestVariant(ALUOp op);

static const ValueOperand JSReturnOperand = ValueOperand(JSReturnReg_Type, JSReturnReg_Data);



















namespace datastore {
struct Reg
{
    
    uint32 RM : 4;
    
    uint32 RRS : 1;
    ShiftType Type : 2;
    
    
    uint32 ShiftAmount : 5;
    uint32 pad : 20;

    Reg(uint32 rm, ShiftType type, uint32 rsr, uint32 shiftamount)
      : RM(rm), RRS(rsr), Type(type), ShiftAmount(shiftamount), pad(0)
    { }

    uint32 encode() {
        return RM | RRS << 4 | Type << 5 | ShiftAmount << 7;
    }
};





struct Imm8mData
{
  private:
    uint32 data : 8;
    uint32 rot : 4;
    
    
    
    uint32 buff : 19;
  public:
    uint32 invalid : 1;

    uint32 encode() {
        JS_ASSERT(!invalid);
        return data | rot << 8;
    };

    
    Imm8mData()
      : data(0xff), rot(0xf), invalid(1)
    { }

    Imm8mData(uint32 data_, uint32 rot_)
      : data(data_), rot(rot_), invalid(0)
    {
        JS_ASSERT(data == data_);
        JS_ASSERT(rot == rot_);
    }
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
    Imm8VFPImmData()
      : imm4L(-1U & 0xf), imm4H(-1U & 0xf), isInvalid(-1)
    { }

    Imm8VFPImmData(uint32 imm)
      : imm4L(imm&0xf), imm4H(imm>>4), isInvalid(0)
    {
        JS_ASSERT(imm <= 0xff);
    }

    uint32 encode() {
        if (isInvalid != 0)
            return -1;
        return imm4L | (imm4H << 16);
    };
};

struct Imm12Data
{
    uint32 data : 12;
    uint32 encode() {
        return data;
    }

    Imm12Data(uint32 imm)
      : data(imm)
    {
        JS_ASSERT(data == imm);
    }

};

struct RIS
{
    uint32 ShiftAmount : 5;
    uint32 encode () {
        return ShiftAmount;
    }

    RIS(uint32 imm)
      : ShiftAmount(imm)
    {
        JS_ASSERT(ShiftAmount == imm);
    }
};

struct RRS
{
    uint32 MustZero : 1;
    
    uint32 RS : 4;

    RRS(uint32 rs)
      : RS(rs)
    {
        JS_ASSERT(rs == RS);
    }

    uint32 encode () {
        return RS << 1;
    }
};

} 

class MacroAssemblerARM;
class Operand;

class Operand2
{
    friend class Operand;
    friend class MacroAssemblerARM;

  public:
    uint32 oper : 31;
    uint32 invalid : 1;

  protected:
    Operand2(datastore::Imm8mData base)
      : oper(base.invalid ? -1 : (base.encode() | (uint32)IsImmOp2)),
        invalid(base.invalid)
    { }

    Operand2(datastore::Reg base)
      : oper(base.encode() | (uint32)IsNotImmOp2)
    { }

  private:
    Operand2(int blob)
      : oper(blob)
    { }

  public:
    uint32 encode() {
        return oper;
    }
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

        TwoImm8mData()
          : fst(), snd()
        { }

        TwoImm8mData(datastore::Imm8mData _fst, datastore::Imm8mData _snd)
          : fst(_fst), snd(_snd)
        { }
    };

    static TwoImm8mData encodeTwoImms(uint32);
    Imm8(uint32 imm)
      : Operand2(encodeImm(imm))
    { }
};

class Op2Reg : public Operand2
{
  public:
    Op2Reg(Register rm, ShiftType type, datastore::RIS shiftImm)
      : Operand2(datastore::Reg(rm.code(), type, 0, shiftImm.encode()))
    { }

    Op2Reg(Register rm, ShiftType type, datastore::RRS shiftReg)
      : Operand2(datastore::Reg(rm.code(), type, 1, shiftReg.encode()))
    { }
};

class O2RegImmShift : public Op2Reg
{
  public:
    O2RegImmShift(Register rn, ShiftType type, uint32 shift)
      : Op2Reg(rn, type, datastore::RIS(shift))
    { }
};

class O2RegRegShift : public Op2Reg
{
  public:
    O2RegRegShift(Register rn, ShiftType type, Register rs)
      : Op2Reg(rn, type, datastore::RRS(rs.code()))
    { }
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
      : data(immdata.encode() | (uint32)IsImmDTR | ((uint32)iu))
    { }

    DtrOff(datastore::Reg reg, IsUp_ iu = IsUp)
      : data(reg.encode() | (uint32) IsNotImmDTR | iu)
    { }

  public:
    uint32 encode() { return data; }
};

class DtrOffImm : public DtrOff
{
  public:
    DtrOffImm(int32 imm)
      : DtrOff(datastore::Imm12Data(abs(imm)), imm >= 0 ? IsUp : IsDown)
    {
        JS_ASSERT((imm < 4096) && (imm > -4096));
    }
};

class DtrOffReg : public DtrOff
{
    
    
  protected:
    DtrOffReg(Register rn, ShiftType type, datastore::RIS shiftImm)
      : DtrOff(datastore::Reg(rn.code(), type, 0, shiftImm.encode()))
    { }

    DtrOffReg(Register rn, ShiftType type, datastore::RRS shiftReg)
      : DtrOff(datastore::Reg(rn.code(), type, 1, shiftReg.encode()))
    { }
};

class DtrRegImmShift : public DtrOffReg
{
  public:
    DtrRegImmShift(Register rn, ShiftType type, uint32 shift)
      : DtrOffReg(rn, type, datastore::RIS(shift))
    { }
};

class DtrRegRegShift : public DtrOffReg
{
  public:
    DtrRegRegShift(Register rn, ShiftType type, Register rs)
      : DtrOffReg(rn, type, datastore::RRS(rs.code()))
    { }
};



class DTRAddr
{
    uint32 data;

  public:
    DTRAddr(Register reg, DtrOff dtr)
      : data(dtr.encode() | (reg.code() << 16))
    { }

    uint32 encode() {
        return data;
    }

  private:
    friend class Operand;
    DTRAddr(uint32 blob)
      : data(blob)
    { }
};



class EDtrOff
{
    uint32 data;

  protected:
    EDtrOff(datastore::Imm8Data imm8, IsUp_ iu = IsUp)
      : data(imm8.encode() | IsImmEDTR | (uint32)iu)
    { }

    EDtrOff(Register rm, IsUp_ iu = IsUp)
      : data(rm.code() | IsNotImmEDTR | iu)
    { }

  public:
    uint32 encode() {
        return data;
    }
};

class EDtrOffImm : public EDtrOff
{
  public:
    EDtrOffImm(int32 imm)
      : EDtrOff(datastore::Imm8Data(abs(imm)), (imm >= 0) ? IsUp : IsDown)
    { }
};




class EDtrOffReg : public EDtrOff
{
  public:
    EDtrOffReg(Register rm)
      : EDtrOff(rm)
    { }
};

class EDtrAddr
{
    uint32 data;

  public:
    EDtrAddr(Register r, EDtrOff off)
      : data(RN(r) | off.encode())
    { }

    uint32 encode() {
        return data;
    }
};

class VFPOff
{
    uint32 data;

  protected:
    VFPOff(datastore::Imm8VFPOffData imm, IsUp_ isup)
      : data(imm.encode() | (uint32)isup)
    { }

  public:
    uint32 encode() {
        return data;
    }
};

class VFPOffImm : public VFPOff
{
  public:
    VFPOffImm(int32 imm)
      : VFPOff(datastore::Imm8VFPOffData(abs(imm) >> 2), imm < 0 ? IsDown : IsUp)
    { }
};
class VFPAddr
{
    friend class Operand;

    uint32 data;

  protected:
    VFPAddr(uint32 blob)
      : data(blob)
    { }

  public:
    VFPAddr(Register base, VFPOff off)
      : data(RN(base) | off.encode())
    { }

    uint32 encode() {
        return data;
    }
};

class VFPImm {
    uint32 data;

  public:
    VFPImm(uint32 top);

    uint32 encode() {
        return data;
    }
    bool isValid() {
        return data != -1U;
    }
};



class BOffImm
{
    uint32 data;

  public:
    uint32 encode() {
        return data;
    }
    int32 decode() {
        return ((((int32)data) << 8) >> 6) + 8;
    }

    explicit BOffImm(int offset)
      : data ((offset - 8) >> 2 & 0x00ffffff)
    {
        JS_ASSERT((offset & 0x3) == 0);
        JS_ASSERT(isInRange(offset));
    }
    static bool isInRange(int offset)
    {
        if ((offset - 8) < -33554432)
            return false;
        if ((offset - 8) > 33554428)
            return false;
        return true;
    }
    static const int INVALID = 0x00800000;
    BOffImm()
      : data(INVALID)
    { }

    bool isInvalid() {
        return data == uint32(INVALID);
    }
    Instruction *getDest(Instruction *src);

  private:
    friend class InstBranchImm;
    BOffImm(Instruction &inst);
};

class Imm16
{
    uint32 lower : 12;
    uint32 pad : 4;
    uint32 upper : 4;
    uint32 invalid : 12;

  public:
    Imm16();
    Imm16(uint32 imm);
    Imm16(Instruction &inst);

    uint32 encode() {
        return lower | upper << 16;
    }
    uint32 decode() {
        return lower | upper << 12;
    }

    bool isInvalid () {
        return invalid;
    }
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
        MEM,
        FOP
    };

  private:
    Tag_ Tag : 3;
    uint32 reg : 5;
    int32 offset;
    uint32 data;

  public:
    Operand (Register reg_)
      : Tag(OP2), reg(reg_.code())
    { }

    Operand (FloatRegister freg)
      : Tag(FOP), reg(freg.code())
    { }

    Operand (Register base, Imm32 off)
      : Tag(MEM), reg(base.code()), offset(off.value)
    { }

    Operand (Register base, int32 off)
      : Tag(MEM), reg(base.code()), offset(off)
    { }

    Operand (const Address &addr)
      : Tag(MEM), reg(addr.base.code()), offset(addr.offset)
    { }

    Tag_ getTag() const {
        return Tag;
    }

    Operand2 toOp2() const {
        JS_ASSERT(Tag == OP2);
        return O2Reg(Register::FromCode(reg));
    }

    void toAddr(Register *r, Imm32 *dest) const {
        JS_ASSERT(Tag == MEM);
        *r = Register::FromCode(reg);
        *dest = Imm32(offset);
    }
    Address toAddress() const {
        return Address(Register::FromCode(reg), offset);
    }
    int32 disp() const {
        JS_ASSERT(Tag == MEM);
        return offset;
    }

    int32 base() const {
        JS_ASSERT(Tag == MEM);
        return reg;
    }
    Register baseReg() const {
        return Register::FromCode(reg);
    }
    DTRAddr toDTRAddr() const {
        return DTRAddr(baseReg(), DtrOffImm(offset));
    }
    VFPAddr toVFPAddr() const {
        return VFPAddr(baseReg(), VFPOffImm(offset));
    }
};

void
PatchJump(CodeLocationJump &jump_, CodeLocationLabel label);

class Assembler;
typedef js::ion::AssemblerBufferWithConstantPool<16, 4, Instruction, Assembler, 1> ARMBuffer;

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
        Unsigned = PL,
        Zero = EQ,
        NonZero = NE,
        Always  = AL,

        VFP_NotEqualOrUnordered = NE,
        VFP_Equal = EQ,
        VFP_Unordered = VS,
        VFP_NotUnordered = VC,
        VFP_GreaterThanOrEqualOrUnordered = CS,
        VFP_GreaterThanOrEqual = GE,
        VFP_GreaterThanOrUnordered = HI,
        VFP_GreaterThan = GT,
        VFP_LessThanOrEqualOrUnordered = LE,
        VFP_LessThanOrEqual = LS,
        VFP_LessThanOrUnordered = LT,
        VFP_LessThan = CC 
    };

    
    
    
    static const int DoubleConditionBitSpecial = 0x1;

    enum DoubleCondition {
        
        DoubleOrdered = VFP_NotUnordered,
        DoubleEqual = VFP_Equal,
        DoubleNotEqual = VFP_NotEqualOrUnordered | DoubleConditionBitSpecial,
        DoubleGreaterThan = VFP_GreaterThan,
        DoubleGreaterThanOrEqual = VFP_GreaterThanOrEqual,
        DoubleLessThan = VFP_LessThan,
        DoubleLessThanOrEqual = VFP_LessThanOrEqual,
        
        DoubleUnordered = VFP_Unordered,
        DoubleEqualOrUnordered = VFP_Equal | DoubleConditionBitSpecial,
        DoubleNotEqualOrUnordered = VFP_NotEqualOrUnordered,
        DoubleGreaterThanOrUnordered = VFP_GreaterThanOrUnordered,
        DoubleGreaterThanOrEqualOrUnordered = VFP_GreaterThanOrEqualOrUnordered,
        DoubleLessThanOrUnordered = VFP_LessThanOrUnordered,
        DoubleLessThanOrEqualOrUnordered = VFP_LessThanOrEqualOrUnordered
    };

    Condition getCondition(uint32 inst) {
        return (Condition) (0xf0000000 & inst);
    }
    static inline Condition ConditionFromDoubleCondition(DoubleCondition cond) {
        JS_ASSERT(!(cond & DoubleConditionBitSpecial));
        return static_cast<Condition>(cond);
    }

    
    

    BufferOffset nextOffset() {
        return m_buffer.nextOffset();
    }

  protected:
    BufferOffset labelOffset (Label *l) {
        return BufferOffset(l->bound());
    }

    Instruction * editSrc (BufferOffset bo) {
        return m_buffer.getInst(bo);
    }
  public:
    void resetCounter();
    uint32 actualOffset(uint32) const;
    uint32 actualIndex(uint32) const;
    static uint8 *PatchableJumpAddress(IonCode *code, uint32 index);
    BufferOffset actualOffset(BufferOffset) const;
  protected:

    
    
    struct RelativePatch
    {
        
        
        BufferOffset offset;
        void *target;
        Relocation::Kind kind;
        void fixOffset(ARMBuffer &m_buffer) {
            offset = BufferOffset(offset.getOffset() + m_buffer.poolSizeBefore(offset.getOffset()));
        }
        RelativePatch(BufferOffset offset, void *target, Relocation::Kind kind)
          : offset(offset),
            target(target),
            kind(kind)
        { }
    };

    
    
    class JumpPool;
    js::Vector<DeferredData *, 0, SystemAllocPolicy> data_;
    js::Vector<CodeLabel *, 0, SystemAllocPolicy> codeLabels_;
    js::Vector<RelativePatch, 8, SystemAllocPolicy> jumps_;
    js::Vector<JumpPool *, 0, SystemAllocPolicy> jumpPools_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpJumpRelocations_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpDataRelocations_;
    class JumpPool : TempObject
    {
        BufferOffset start;
        uint32 size;
        bool fixup(IonCode *code, uint8 *data);
    };

    CompactBufferWriter jumpRelocations_;
    CompactBufferWriter dataRelocations_;
    CompactBufferWriter relocations_;
    size_t dataBytesNeeded_;

    bool enoughMemory_;

    
    ARMBuffer m_buffer;

    
    
    
    
    
    
    
    
    
    static Assembler *dummy;
    Pool pools_[4];
    Pool *int32Pool;
    Pool *doublePool;

  public:
    Assembler()
      : dataBytesNeeded_(0),
        enoughMemory_(true),
        m_buffer(4, 4, 0, 2, &pools_[0], 8),
        int32Pool(m_buffer.getPool(1)),
        doublePool(m_buffer.getPool(0)),
        isFinished(false),
        dtmActive(false),
        dtmCond(Always)
    {
        
        new (&pools_[2]) Pool (1024, 8, 4, 8, 8, true);
        
        new (&pools_[3]) Pool (4096, 4, 4, 4, 4, true, true);
        
        new (doublePool) Pool (1024, 8, 4, 8, 8, false, false, &pools_[2]);
        
        new (int32Pool) Pool (4096, 4, 4, 4, 4, false, true, &pools_[3]);
        for (int i = 0; i < 4; i++) {
            if (pools_[i].poolData == NULL) {
                m_buffer.fail_oom();
                return;
            }
        }
    }
    static Condition InvertCondition(Condition cond);

    
    void trace(JSTracer *trc);
    void writeRelocation(BufferOffset src) {
        tmpJumpRelocations_.append(src);
    }

    
    
    void writeDataRelocation(const ImmGCPtr &ptr) {
        if (ptr.value)
            tmpDataRelocations_.append(nextOffset());
    }

    enum RelocBranchStyle {
        B_MOVWT,
        B_LDR_BX,
        B_LDR,
        B_MOVW_ADD
    };

    enum RelocStyle {
        L_MOVWT,
        L_LDR
    };

  public:
    
    
    
    static const uint32 * getCF32Target(Instruction *jump);

    static uintptr_t getPointer(uint8 *);
    static const uint32 * getPtr32Target(Instruction *load, Register *dest = NULL, RelocStyle *rs = NULL);

    bool oom() const;
  private:
    bool isFinished;
  public:
    void finish();
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

    
    
    
    
    
    void writeInst(uint32 x, uint32 *dest = NULL);
    
    
    static void writeInstStatic(uint32 x, uint32 *dest);

  public:
    
    
    void as_jumpPool(uint32 size);

    void align(int alignment);
    void as_nop();
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

    
    
    
    void as_movw(Register dest, Imm16 imm, Condition c = Always, Instruction *pos = NULL);
    void as_movt(Register dest, Imm16 imm, Condition c = Always, Instruction *pos = NULL);

    void as_genmul(Register d1, Register d2, Register rm, Register rn,
                   MULOp op, SetCond_ sc, Condition c = Always);
    void as_mul(Register dest, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    void as_mla(Register dest, Register acc, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    void as_umaal(Register dest1, Register dest2, Register src1, Register src2,
                  Condition c = Always);
    void as_mls(Register dest, Register acc, Register src1, Register src2,
                Condition c = Always);
    void as_umull(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    void as_umlal(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    void as_smull(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    void as_smlal(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    
    
    
    void as_dtr(LoadStore ls, int size, Index mode,
                Register rt, DTRAddr addr, Condition c = Always, uint32 *dest = NULL);
    
    
    
    void as_extdtr(LoadStore ls, int size, bool IsSigned, Index mode,
                   Register rt, EDtrAddr addr, Condition c = Always, uint32 *dest = NULL);

    void as_dtm(LoadStore ls, Register rn, uint32 mask,
                DTMMode mode, DTMWriteBack wb, Condition c = Always);
    
    ARMBuffer::PoolEntry as_Imm32Pool(Register dest, uint32 value, Condition c = Always);
    
    ARMBuffer::PoolEntry as_BranchPool(uint32 value, RepatchLabel *label, Condition c);

    
    ARMBuffer::PoolEntry as_FImm64Pool(VFPRegister dest, double value, Condition c = Always);
    

    
    
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

    void writeVFPInst(vfp_size sz, uint32 blob, uint32 *dest=NULL);
    
    
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
                  Condition c = Always, int idx = 0);

    
    
    void as_vcvt(VFPRegister vd, VFPRegister vm, bool useFPSCR = false,
                 Condition c = Always);
    
    void as_vcvtFixed(VFPRegister vd, bool isSigned, uint32 fixedPoint, bool toFixed, Condition c = Always);

    
    void as_vdtr(LoadStore ls, VFPRegister vd, VFPAddr addr,
                 Condition c = Always ,
                 uint32 *dest = NULL);

    
    

    void as_vdtm(LoadStore st, Register rn, VFPRegister vd, int length,
                 Condition c = Always);

    void as_vimm(VFPRegister vd, VFPImm imm, Condition c = Always);

    void as_vmrs(Register r, Condition c = Always);
    
    bool nextLink(BufferOffset b, BufferOffset *next);
    void bind(Label *label, BufferOffset boff = BufferOffset());
    void bind(RepatchLabel *label);
    uint32 currentOffset() {
        return nextOffset().getOffset();
    }
    void retarget(Label *label, Label *target);
    
    void retarget(Label *label, void *target, Relocation::Kind reloc);

    void call(Label *label);
    void call(void *target);

    void as_bkpt();

  public:
    static void TraceJumpRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);
    static void TraceDataRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);

  protected:
    void addPendingJump(BufferOffset src, void *target, Relocation::Kind kind) {
        enoughMemory_ &= jumps_.append(RelativePatch(src, target, kind));
        if (kind == Relocation::IONCODE)
            writeRelocation(src);
    }

  public:
    
    
    void flush() {
        JS_ASSERT(!isFinished);
        m_buffer.flushPool();
        return;
    }

    
    
    void executableCopy(uint8 *buffer);

    

    
    
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
        dtmMode = mode;
    }
    void transferFloatReg(VFPRegister rn)
    {
        if (dtmLastReg == -1) {
            vdtmFirstReg = rn;
        } else {
            JS_ASSERT(dtmLastReg >= 0);
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

    
    static uint32 patchConstantPoolLoad(uint32 load, int32 value);
    static void insertTokenIntoTag(uint32 size, uint8 *load, int32 token);
    
    
    static void patchConstantPoolLoad(void* loadAddr, void* constPoolAddr);
    
    
    
    static uint32 placeConstantPoolBarrier(int offset);
    
    
    
    void dumpPool();
    void flushBuffer();
    void enterNoPool();
    void leaveNoPool();
    
    
    static ptrdiff_t getBranchOffset(const Instruction *i);
    static void retargetNearBranch(Instruction *i, int offset, Condition cond);
    static void retargetNearBranch(Instruction *i, int offset);
    static void retargetFarBranch(Instruction *i, uint8 **slot, uint8 *dest, Condition cond);

    static void writePoolHeader(uint8 *start, Pool *p, bool isNatural);
    static void writePoolFooter(uint8 *start, Pool *p, bool isNatural);
    static void writePoolGuard(BufferOffset branch, Instruction *inst, BufferOffset dest);


    static uint32 patchWrite_NearCallSize();
    static uint32 nopSize() { return 4; }
    static void patchWrite_NearCall(CodeLocationLabel start, CodeLocationLabel toCall);
    static void patchDataWithValueCheck(CodeLocationLabel label, ImmWord newValue,
                                        ImmWord expectedValue);
    static void patchWrite_Imm32(CodeLocationLabel label, Imm32 imm);
    static uint32 alignDoubleArg(uint32 offset) {
        return (offset+1)&~1;
    }
    static uint8 *nextInstruction(uint8 *instruction, uint32 *count = NULL);
}; 



class Instruction
{
    uint32 data;

  protected:
    
    
    Instruction (uint32 data_, bool fake = false) : data(data_ | 0xf0000000) {
        JS_ASSERT (fake || ((data_ & 0xf0000000) == 0));
    }
    
    Instruction (uint32 data_, Assembler::Condition c) : data(data_ | (uint32) c) {
        JS_ASSERT ((data_ & 0xf0000000) == 0);
    }
    
    
    
  public:
    uint32 encode() const {
        return data;
    }
    
    template <class C>
    bool is() const { return C::isTHIS(*this); }

    
    template <class C>
    C *as() const { return C::asTHIS(*this); }

    const Instruction & operator=(const Instruction &src) {
        data = src.data;
        return *this;
    }
    
    
    void extractCond(Assembler::Condition *c) {
        if (data >> 28 != 0xf )
            *c = (Assembler::Condition)(data & 0xf0000000);
    }
    
    
    Instruction *next();

    
    
    const uint32 *raw() const { return &data; }
}; 


JS_STATIC_ASSERT(sizeof(Instruction) == 4);


class InstDTR : public Instruction
{
  public:
    enum IsByte_ {
        IsByte = 0x00400000,
        IsWord = 0x00000000
    };
    static const int IsDTR     = 0x04000000;
    static const int IsDTRMask = 0x0c000000;

    
    InstDTR(LoadStore ls, IsByte_ ib, Index mode, Register rt, DTRAddr addr, Assembler::Condition c)
      : Instruction(ls | ib | mode | RT(rt) | addr.encode() | IsDTR, c)
    { }

    static bool isTHIS(const Instruction &i);
    static InstDTR *asTHIS(Instruction &i);

};
JS_STATIC_ASSERT(sizeof(InstDTR) == sizeof(Instruction));

class InstLDR : public InstDTR
{
  public:
    InstLDR(Index mode, Register rt, DTRAddr addr, Assembler::Condition c)
        : InstDTR(IsLoad, IsWord, mode, rt, addr, c)
    { }
    static bool isTHIS(const Instruction &i);
    static InstLDR *asTHIS(Instruction &i);

};
JS_STATIC_ASSERT(sizeof(InstDTR) == sizeof(InstLDR));

class InstBranchReg : public Instruction
{
  protected:
    
    enum BranchTag {
        IsBX  = 0x012fff10,
        IsBLX = 0x012fff30
    };
    static const uint32 IsBRegMask = 0x0ffffff0;
    InstBranchReg(BranchTag tag, Register rm, Assembler::Condition c)
      : Instruction(tag | RM(rm), c)
    { }
  public:
    static bool isTHIS (const Instruction &i);
    static InstBranchReg *asTHIS (const Instruction &i);
    
    void extractDest(Register *dest);
    
    bool checkDest(Register dest);
};
JS_STATIC_ASSERT(sizeof(InstBranchReg) == sizeof(Instruction));


class InstBranchImm : public Instruction
{
  protected:
    enum BranchTag {
        IsB   = 0x0a000000,
        IsBL  = 0x0b000000
    };
    static const uint32 IsBImmMask = 0x0f000000;

    InstBranchImm(BranchTag tag, BOffImm off, Assembler::Condition c)
      : Instruction(tag | off.encode(), c)
    { }

  public:
    static bool isTHIS (const Instruction &i);
    static InstBranchImm *asTHIS (const Instruction &i);
    void extractImm(BOffImm *dest);
};
JS_STATIC_ASSERT(sizeof(InstBranchImm) == sizeof(Instruction));


class InstBXReg : public InstBranchReg
{
  public:
    static bool isTHIS (const Instruction &i);
    static InstBXReg *asTHIS (const Instruction &i);
};
class InstBLXReg : public InstBranchReg
{
  public:
    static bool isTHIS (const Instruction &i);
    static InstBLXReg *asTHIS (const Instruction &i);
};
class InstBImm : public InstBranchImm
{
  public:
    InstBImm(BOffImm off, Assembler::Condition c)
      : InstBranchImm(IsB, off, c)
    { }

    static bool isTHIS (const Instruction &i);
    static InstBImm *asTHIS (const Instruction &i);
};
class InstBLImm : public InstBranchImm
{
  public:
    InstBLImm(BOffImm off, Assembler::Condition c)
      : InstBranchImm(IsBL, off, c)
    { }

    static bool isTHIS (const Instruction &i);
    static InstBLImm *asTHIS (Instruction &i);
};



class InstMovWT : public Instruction
{
  protected:
    enum WT {
        IsW = 0x03000000,
        IsT = 0x03400000
    };
    static const uint32 IsWTMask = 0x0ff00000;

    InstMovWT(Register rd, Imm16 imm, WT wt, Assembler::Condition c)
      : Instruction (RD(rd) | imm.encode() | wt, c)
    { }

  public:
    void extractImm(Imm16 *dest);
    void extractDest(Register *dest);
    bool checkImm(Imm16 dest);
    bool checkDest(Register dest);

    static bool isTHIS (Instruction &i);
    static InstMovWT *asTHIS (Instruction &i);

};
JS_STATIC_ASSERT(sizeof(InstMovWT) == sizeof(Instruction));

class InstMovW : public InstMovWT
{
  public:
    InstMovW (Register rd, Imm16 imm, Assembler::Condition c)
      : InstMovWT(rd, imm, IsW, c)
    { }

    static bool isTHIS (const Instruction &i);
    static InstMovW *asTHIS (const Instruction &i);
};

class InstMovT : public InstMovWT
{
  public:
    InstMovT (Register rd, Imm16 imm, Assembler::Condition c)
      : InstMovWT(rd, imm, IsT, c)
    { }
    static bool isTHIS (const Instruction &i);
    static InstMovT *asTHIS (const Instruction &i);
};
static const uint32 NumArgRegs = 4;
static inline bool
GetArgReg(uint32 arg, Register *out)
{
    if (arg < 4) {
        *out = Register::FromCode(arg);
        return true;
    }
    return false;
}

static inline uint32
GetArgStackDisp(uint32 arg)
{
    JS_ASSERT(arg >= NumArgRegs);
    return (arg - NumArgRegs) * STACK_SLOT_SIZE;
}

class DoubleEncoder {
    uint32 rep(bool b, uint32 count) {
        uint32 ret = 0;
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

    struct DoubleEntry
    {
        uint32 dblTop;
        datastore::Imm8VFPImmData data;

        DoubleEntry()
          : dblTop(-1)
        { }
        DoubleEntry(uint32 dblTop_, datastore::Imm8VFPImmData data_)
          : dblTop(dblTop_), data(data_)
        { }
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

class DePooler {
    Assembler *masm_;
  public:
    DePooler(Assembler *masm) : masm_(masm) {
        masm_->enterNoPool();
    }
    ~DePooler() {
        masm_->leaveNoPool();
    }
};

} 
} 

#endif
