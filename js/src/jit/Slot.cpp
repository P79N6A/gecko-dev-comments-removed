






#include "jit/Slot.h"

#include "jit/CompactBuffer.h"

using namespace js;
using namespace js::jit;


























































#ifdef JS_NUNBOX32
static const uint32_t NUNBOX32_STACK_STACK = 0;
static const uint32_t NUNBOX32_STACK_REG   = 1;
static const uint32_t NUNBOX32_REG_STACK   = 2;
static const uint32_t NUNBOX32_REG_REG     = 3;
#endif

static const uint32_t MAX_TYPE_FIELD_VALUE = 7;

static const uint32_t MAX_REG_FIELD_VALUE         = 31;
static const uint32_t ESC_REG_FIELD_INDEX         = 31;
static const uint32_t ESC_REG_FIELD_CONST         = 30;
static const uint32_t ESC_REG_FIELD_FLOAT32_STACK = 29;
static const uint32_t ESC_REG_FIELD_FLOAT32_REG   = 28;
static const uint32_t MIN_REG_FIELD_ESC           = 28;

Slot
Slot::read(CompactBufferReader &reader)
{
    uint8_t b = reader.readByte();

    JSValueType type = JSValueType(b & 0x7);
    uint32_t code = b >> 3;

    switch (type) {
      case JSVAL_TYPE_DOUBLE:
        if (code < MIN_REG_FIELD_ESC)
            return DoubleSlot(FloatRegister::FromCode(code));
        JS_ASSERT(code == ESC_REG_FIELD_INDEX);
        return TypedSlot(type, reader.readSigned());

      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_STRING:
      case JSVAL_TYPE_OBJECT:
      case JSVAL_TYPE_BOOLEAN:
        if (code < MIN_REG_FIELD_ESC)
            return TypedSlot(type, Register::FromCode(code));
        JS_ASSERT(code == ESC_REG_FIELD_INDEX);
        return TypedSlot(type, reader.readSigned());

      case JSVAL_TYPE_NULL:
        if (code == ESC_REG_FIELD_CONST)
            return NullSlot();
        if (code == ESC_REG_FIELD_INDEX)
            return Int32Slot(reader.readSigned());
        if (code == ESC_REG_FIELD_FLOAT32_REG)
            return Float32Slot(FloatRegister::FromCode(reader.readUnsigned()));
        if (code == ESC_REG_FIELD_FLOAT32_STACK)
            return Float32Slot(reader.readSigned());
        return Int32Slot(code);

      case JSVAL_TYPE_UNDEFINED:
        if (code == ESC_REG_FIELD_CONST)
            return UndefinedSlot();
        if (code == ESC_REG_FIELD_INDEX)
            return ConstantPoolSlot(reader.readUnsigned());
        return ConstantPoolSlot(code);

      case JSVAL_TYPE_MAGIC:
      {
        if (code == ESC_REG_FIELD_CONST) {
            uint8_t reg2 = reader.readUnsigned();
            if (reg2 != ESC_REG_FIELD_INDEX)
                return TypedSlot(type, Register::FromCode(reg2));
            return TypedSlot(type, reader.readSigned());
        }

#ifdef JS_NUNBOX32
        int32_t type, payload;
        switch (code) {
          case NUNBOX32_STACK_STACK:
            type = reader.readSigned();
            payload = reader.readSigned();
            return UntypedSlot(type, payload);
          case NUNBOX32_STACK_REG:
            type = reader.readSigned();
            payload = reader.readByte();
            return UntypedSlot(type, Register::FromCode(payload));
          case NUNBOX32_REG_STACK:
            type = reader.readByte();
            payload = reader.readSigned();
            return UntypedSlot(Register::FromCode(type), payload);
          case NUNBOX32_REG_REG:
            type = reader.readByte();
            payload = reader.readByte();
            return UntypedSlot(Register::FromCode(type),
                               Register::FromCode(payload));
          default:
            MOZ_ASSUME_UNREACHABLE("bad code");
            break;
        }
#elif JS_PUNBOX64
        if (code < MIN_REG_FIELD_ESC)
            return UntypedSlot(Register::FromCode(code));
         JS_ASSERT(code == ESC_REG_FIELD_INDEX);
         return UntypedSlot(reader.readSigned());
#endif
      }

      default:
        MOZ_ASSUME_UNREACHABLE("bad type");
        break;
    }

    MOZ_ASSUME_UNREACHABLE("huh?");
}

void
Slot::writeSlotHeader(CompactBufferWriter &writer, JSValueType type, uint32_t regCode) const
{
    JS_ASSERT(uint32_t(type) <= MAX_TYPE_FIELD_VALUE);
    JS_ASSERT(uint32_t(regCode) <= MAX_REG_FIELD_VALUE);
    JS_STATIC_ASSERT(Registers::Total < MIN_REG_FIELD_ESC);

    uint8_t byte = uint32_t(type) | (regCode << 3);
    writer.writeByte(byte);
}

void
Slot::write(CompactBufferWriter &writer) const
{
    switch (mode()) {
      case CONSTANT: {
        if (constantIndex() < MIN_REG_FIELD_ESC) {
            writeSlotHeader(writer, JSVAL_TYPE_UNDEFINED, constantIndex());
        } else {
            writeSlotHeader(writer, JSVAL_TYPE_UNDEFINED, ESC_REG_FIELD_INDEX);
            writer.writeUnsigned(constantIndex());
        }
        break;
      }

      case DOUBLE_REG: {
        writeSlotHeader(writer, JSVAL_TYPE_DOUBLE, floatReg().code());
        break;
      }

      case FLOAT32_REG: {
        writeSlotHeader(writer, JSVAL_TYPE_NULL, ESC_REG_FIELD_FLOAT32_REG);
        writer.writeUnsigned(floatReg().code());
        break;
      }

      case FLOAT32_STACK: {
        writeSlotHeader(writer, JSVAL_TYPE_NULL, ESC_REG_FIELD_FLOAT32_STACK);
        writer.writeSigned(stackSlot());
        break;
      }

      case TYPED_REG: {
        writeSlotHeader(writer, knownType(), reg().code());
        break;
      }
      case TYPED_STACK: {
        writeSlotHeader(writer, knownType(), ESC_REG_FIELD_INDEX);
        writer.writeSigned(stackSlot());
        break;
      }
      case UNTYPED: {
#if defined(JS_NUNBOX32)
        uint32_t code = 0;
        if (type().isStackSlot()) {
            if (payload().isStackSlot())
                code = NUNBOX32_STACK_STACK;
            else
                code = NUNBOX32_STACK_REG;
        } else {
            if (payload().isStackSlot())
                code = NUNBOX32_REG_STACK;
            else
                code = NUNBOX32_REG_REG;
        }

        writeSlotHeader(writer, JSVAL_TYPE_MAGIC, code);

        if (type().isStackSlot())
            writer.writeSigned(type().stackSlot());
        else
            writer.writeByte(type().reg().code());

        if (payload().isStackSlot())
            writer.writeSigned(payload().stackSlot());
        else
            writer.writeByte(payload().reg().code());

#elif defined(JS_PUNBOX64)
        if (value().isStackSlot()) {
            writeSlotHeader(writer, JSVAL_TYPE_MAGIC, ESC_REG_FIELD_INDEX);
            writer.writeSigned(value().stackSlot());
        } else {
            writeSlotHeader(writer, JSVAL_TYPE_MAGIC, value().reg().code());
        }
#endif
        break;
      }
      case JS_UNDEFINED: {
        writeSlotHeader(writer, JSVAL_TYPE_UNDEFINED, ESC_REG_FIELD_CONST);
        break;
      }
      case JS_NULL: {
        writeSlotHeader(writer, JSVAL_TYPE_NULL, ESC_REG_FIELD_CONST);
        break;
      }
      case JS_INT32: {
        if (int32Value() >= 0 && uint32_t(int32Value()) < MIN_REG_FIELD_ESC) {
            writeSlotHeader(writer, JSVAL_TYPE_NULL, int32Value());
        } else {
            writeSlotHeader(writer, JSVAL_TYPE_NULL, ESC_REG_FIELD_INDEX);
            writer.writeSigned(int32Value());
        }
        break;
      }
      case INVALID: {
        MOZ_ASSUME_UNREACHABLE("not initialized");
        break;
      }
    }
}

void
Location::dump(FILE *fp) const
{
    if (isStackSlot())
        fprintf(fp, "stack %d", stackSlot());
    else
        fprintf(fp, "reg %s", reg().name());
}

static const char *
ValTypeToString(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_INT32:
        return "int32_t";
      case JSVAL_TYPE_DOUBLE:
        return "double";
      case JSVAL_TYPE_STRING:
        return "string";
      case JSVAL_TYPE_BOOLEAN:
        return "boolean";
      case JSVAL_TYPE_OBJECT:
        return "object";
      case JSVAL_TYPE_MAGIC:
        return "magic";
      default:
        MOZ_ASSUME_UNREACHABLE("no payload");
    }
}

void
Slot::dump(FILE *fp) const
{
    switch (mode()) {
      case CONSTANT:
        fprintf(fp, "constant (pool index %u)", constantIndex());
        break;

      case DOUBLE_REG:
        fprintf(fp, "double (reg %s)", floatReg().name());
        break;

      case FLOAT32_REG:
        fprintf(fp, "float32 (reg %s)", floatReg().name());
        break;

      case FLOAT32_STACK:
        fprintf(fp, "float32 (");
        known_type_.payload.dump(fp);
        fprintf(fp, ")");
        break;

      case TYPED_REG:
      case TYPED_STACK:
        fprintf(fp, "%s (", ValTypeToString(knownType()));
        known_type_.payload.dump(fp);
        fprintf(fp, ")");
        break;

      case UNTYPED: {
        fprintf(fp, "value (");
#if defined(JS_NUNBOX32)
        fprintf(fp, "type = ");
        type().dump(fp);
        fprintf(fp, ", payload = ");
        payload().dump(fp);
#elif defined(JS_PUNBOX64)
        value().dump(fp);
#endif
        fprintf(fp, ")");
        break;
      }

      case JS_UNDEFINED:
        fprintf(fp, "undefined");
        break;

      case JS_NULL:
        fprintf(fp, "null");
        break;

      case JS_INT32:
        fprintf(fp, "int32_t %d", int32Value());
        break;

      case INVALID:
        fprintf(fp, "invalid");
        break;
    }
}
