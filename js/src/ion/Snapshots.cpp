








































#include "MIRGenerator.h"
#include "Snapshots.h"
#include "jsscript.h"
#include "IonLinker.h"

using namespace js;
using namespace js::ion;





























































SnapshotReader::SnapshotReader(const uint8 *buffer, const uint8 *end)
  : reader_(buffer, end)
#ifdef DEBUG
    , slotsRead_(0)
#endif
{
#ifdef DEBUG
    union {
        JSScript *script;
        uint8 bytes[sizeof(JSScript *)];
    } u;
    for (size_t i = 0; i < sizeof(JSScript *); i++)
        u.bytes[i] = reader_.readByte();
    script_ = u.script;
#endif

    pcOffset_ = reader_.readUnsigned();
    slotCount_ = reader_.readUnsigned();
}

#ifdef JS_NUNBOX32
static const uint32 NUNBOX32_STACK_STACK = 0;
static const uint32 NUNBOX32_STACK_REG   = 1;
static const uint32 NUNBOX32_REG_STACK   = 2;
static const uint32 NUNBOX32_REG_REG     = 3;
#endif

static const uint32 MAX_TYPE_FIELD_VALUE = 7;
static const uint32 MAX_REG_FIELD_VALUE  = 31;


static const uint32 SINGLETON_VALUE      = 30;

SnapshotReader::Slot
SnapshotReader::readSlot()
{
    JS_ASSERT(slotsRead_ < slotCount_);
#ifdef DEBUG
    slotsRead_++;
#endif

    uint8 b = reader_.readByte();

    JSValueType type = JSValueType(b & 0x7);
    uint32 code = b >> 3;

    switch (type) {
      case JSVAL_TYPE_DOUBLE:
        if (code != FloatRegisters::Invalid)
            return Slot(TYPED_REG, type, Location::From(Register::FromCode(code)));
        return Slot(FloatRegister::FromCode(code));

      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_STRING:
      case JSVAL_TYPE_OBJECT:
      case JSVAL_TYPE_BOOLEAN:
        if (code != Registers::Invalid)
            return Slot(TYPED_REG, type, Location::From(Register::FromCode(code)));
        return Slot(TYPED_STACK, type, Location::From(reader_.readSigned()));

      case JSVAL_TYPE_NULL:
        if (code == SINGLETON_VALUE)
            return Slot(JS_NULL);
        if (code == MAX_REG_FIELD_VALUE)
            return Slot(JS_INT32, reader_.readSigned());
        return Slot(JS_INT32, code);

      case JSVAL_TYPE_UNDEFINED:
        if (code == SINGLETON_VALUE)
            return Slot(JS_UNDEFINED);
        if (code == MAX_REG_FIELD_VALUE)
            return Slot(CONSTANT, reader_.readUnsigned());
        return Slot(CONSTANT, code);

      default:
      {
        JS_ASSERT(type == JSVAL_TYPE_MAGIC);
        Slot slot(UNTYPED);
#ifdef JS_NUNBOX32
        switch (code) {
          case NUNBOX32_STACK_STACK:
            slot.unknown_type_.type = Location::From(reader_.readSigned());
            slot.unknown_type_.payload = Location::From(reader_.readSigned());
            return slot;
          case NUNBOX32_STACK_REG:
            slot.unknown_type_.type = Location::From(reader_.readSigned());
            slot.unknown_type_.payload = Location::From(Register::FromCode(reader_.readByte()));
            return slot;
          case NUNBOX32_REG_STACK:
            slot.unknown_type_.type = Location::From(Register::FromCode(reader_.readByte()));
            slot.unknown_type_.payload = Location::From(reader_.readSigned());
            return slot;
          default:
            JS_ASSERT(code == NUNBOX32_REG_REG);
            slot.unknown_type_.type = Location::From(Register::FromCode(reader_.readByte()));
            slot.unknown_type_.payload = Location::From(Register::FromCode(reader_.readByte()));
            return slot;
        }
#elif JS_PUNBOX64
        if (code != Registers::Invalid)
            slot.unknown_type_.value = Location::From(Register::FromCode(code));
        else
            slot.unknown_type_.value = Location::From(reader_.readSigned());
        return slot;
#endif
      }
    }

    JS_NOT_REACHED("huh?");
    return Slot(JS_UNDEFINED);
}

void
SnapshotReader::finishReading()
{
    JS_ASSERT(slotsRead_ == slotCount_);
    JS_ASSERT(reader_.readSigned() == -1);
}

SnapshotWriter::SnapshotWriter()
  : nslots_(0),
    slotsWritten_(0)
{
}

SnapshotOffset
SnapshotWriter::start(JSFunction *fun, JSScript *script, jsbytecode *pc,
                      uint32 frameSize, uint32 exprStack)
{
    JS_ASSERT(CountArgSlots(fun) < SNAPSHOT_MAX_NARGS);
    JS_ASSERT(exprStack < SNAPSHOT_MAX_STACK);

    uint32 formalArgs = CountArgSlots(fun);

    nslots_ = formalArgs + script->nfixed + exprStack;
    slotsWritten_ = 0;

    SnapshotOffset offset = writer_.length();

#ifdef DEBUG
    union {
        JSScript *script;
        uint8 bytes[sizeof(JSScript *)];
    } u;
    u.script = script;
    for (size_t i = 0; i < sizeof(JSScript *); i++)
        writer_.writeByte(u.bytes[i]);
#endif

    writer_.writeUnsigned(uint32(pc - script->code));
    writer_.writeUnsigned(nslots_);

    return offset;
}

void
SnapshotWriter::writeSlotHeader(JSValueType type, uint32 regCode)
{
    JS_ASSERT(uint32(type) <= MAX_TYPE_FIELD_VALUE);
    JS_ASSERT(uint32(regCode) <= MAX_REG_FIELD_VALUE);

    uint8 byte = uint32(type) | (regCode << 3);
    writer_.writeByte(byte);

    slotsWritten_++;
    JS_ASSERT(slotsWritten_ <= nslots_);
}

void
SnapshotWriter::addSlot(const FloatRegister &reg)
{
    writeSlotHeader(JSVAL_TYPE_DOUBLE, reg.code());
}

void
SnapshotWriter::addSlot(JSValueType type, const Register &reg)
{
    JS_ASSERT(type != JSVAL_TYPE_DOUBLE);
    writeSlotHeader(type, reg.code());
}

void
SnapshotWriter::addSlot(JSValueType type, int32 stackOffset)
{
    if (type == JSVAL_TYPE_DOUBLE)
        writeSlotHeader(type, FloatRegisters::Invalid);
    else
        writeSlotHeader(type, Registers::Invalid);
    writer_.writeSigned(stackOffset);
}

#if defined(JS_NUNBOX32)
void
SnapshotWriter::addSlot(const Register &type, const Register &payload)
{
    writeSlotHeader(JSVAL_TYPE_MAGIC, NUNBOX32_REG_REG);
    writer_.writeByte(type.code());
    writer_.writeByte(payload.code());
}

void
SnapshotWriter::addSlot(const Register &type, int32 payloadStackOffset)
{
    writeSlotHeader(JSVAL_TYPE_MAGIC, NUNBOX32_REG_STACK);
    writer_.writeByte(type.code());
    writer_.writeSigned(payloadStackOffset);
}

void
SnapshotWriter::addSlot(int32 typeStackOffset, const Register &payload)
{
    writeSlotHeader(JSVAL_TYPE_MAGIC, NUNBOX32_STACK_REG);
    writer_.writeSigned(typeStackOffset);
    writer_.writeByte(payload.code());
}

void
SnapshotWriter::addSlot(int32 typeStackOffset, int32 payloadStackOffset)
{
    writeSlotHeader(JSVAL_TYPE_MAGIC, NUNBOX32_STACK_STACK);
    writer_.writeSigned(typeStackOffset);
    writer_.writeSigned(payloadStackOffset);
}

#elif defined(JS_PUNBOX64)
void
SnapshotWriter::addSlot(const Register &value)
{
    writeSlotHeader(JSVAL_TYPE_MAGIC, value.code());
}

void
SnapshotWriter::addSlot(int32 valueStackSlot)
{
    writeSlotHeader(JSVAL_TYPE_MAGIC, Registers::Invalid);
    writer_.writeSigned(valueStackSlot);
}
#endif

void
SnapshotWriter::addUndefinedSlot()
{
    writeSlotHeader(JSVAL_TYPE_UNDEFINED, SINGLETON_VALUE);
}

void
SnapshotWriter::addNullSlot()
{
    writeSlotHeader(JSVAL_TYPE_NULL, SINGLETON_VALUE);
}

void
SnapshotWriter::endSnapshot()
{
    
    JS_ASSERT(nslots_ == slotsWritten_);

    
#ifdef DEBUG
    writer_.writeSigned(-1);
#endif
}

void
SnapshotWriter::addInt32Slot(int32 value)
{
    if (value >= 0 && uint32(value) < SINGLETON_VALUE) {
        writeSlotHeader(JSVAL_TYPE_NULL, value);
    } else {
        writeSlotHeader(JSVAL_TYPE_NULL, MAX_REG_FIELD_VALUE);
        writer_.writeSigned(value);
    }
}

void
SnapshotWriter::addConstantPoolSlot(uint32 index)
{
    if (index >= 0 && uint32(index) < SINGLETON_VALUE) {
        writeSlotHeader(JSVAL_TYPE_UNDEFINED, index);
    } else {
        writeSlotHeader(JSVAL_TYPE_UNDEFINED, MAX_REG_FIELD_VALUE);
        writer_.writeUnsigned(index);
    }
}

