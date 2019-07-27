





#ifndef jit_Compactbuffer_h
#define jit_Compactbuffer_h

#include "jsalloc.h"

#include "jit/IonTypes.h"
#include "js/Vector.h"

namespace js {
namespace jit {

class CompactBufferWriter;










class CompactBufferReader
{
    const uint8_t *buffer_;
    const uint8_t *end_;

    uint32_t readVariableLength() {
        uint32_t val = 0;
        uint32_t shift = 0;
        uint8_t byte;
        while (true) {
            JS_ASSERT(shift < 32);
            byte = readByte();
            val |= (uint32_t(byte) >> 1) << shift;
            shift += 7;
            if (!(byte & 1))
                return val;
        }
    }

  public:
    CompactBufferReader(const uint8_t *start, const uint8_t *end)
      : buffer_(start),
        end_(end)
    { }
    inline explicit CompactBufferReader(const CompactBufferWriter &writer);
    uint8_t readByte() {
        JS_ASSERT(buffer_ < end_);
        return *buffer_++;
    }
    uint32_t readFixedUint32_t() {
        uint32_t b0 = readByte();
        uint32_t b1 = readByte();
        uint32_t b2 = readByte();
        uint32_t b3 = readByte();
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }
    uint16_t readFixedUint16_t() {
        uint32_t b0 = readByte();
        uint32_t b1 = readByte();
        return b0 | (b1 << 8);
    }
    uint32_t readNativeEndianUint32_t() {
        
        JS_ASSERT(uintptr_t(buffer_) % sizeof(uint32_t) == 0);
        return *reinterpret_cast<const uint32_t *>(buffer_);
    }
    uint32_t readUnsigned() {
        return readVariableLength();
    }
    int32_t readSigned() {
        uint8_t b = readByte();
        bool isNegative = !!(b & (1 << 0));
        bool more = !!(b & (1 << 1));
        int32_t result = b >> 2;
        if (more)
            result |= readUnsigned() << 6;
        if (isNegative)
            return -result;
        return result;
    }

    bool more() const {
        JS_ASSERT(buffer_ <= end_);
        return buffer_ < end_;
    }

    void seek(const uint8_t *start, uint32_t offset) {
        buffer_ = start + offset;
        MOZ_ASSERT(start < end_);
        MOZ_ASSERT(buffer_ < end_);
    }

    const uint8_t *currentPosition() const {
        return buffer_;
    }
};

class CompactBufferWriter
{
    js::Vector<uint8_t, 32, SystemAllocPolicy> buffer_;
    bool enoughMemory_;

  public:
    CompactBufferWriter()
      : enoughMemory_(true)
    { }

    
    
    void writeByte(uint32_t byte) {
        JS_ASSERT(byte <= 0xFF);
        enoughMemory_ &= buffer_.append(byte);
    }
    void writeUnsigned(uint32_t value) {
        do {
            uint8_t byte = ((value & 0x7F) << 1) | (value > 0x7F);
            writeByte(byte);
            value >>= 7;
        } while (value);
    }
    void writeSigned(int32_t v) {
        bool isNegative = v < 0;
        uint32_t value = isNegative ? -v : v;
        uint8_t byte = ((value & 0x3F) << 2) | ((value > 0x3F) << 1) | uint32_t(isNegative);
        writeByte(byte);

        
        value >>= 6;
        if (value == 0)
            return;
        writeUnsigned(value);
    }
    void writeFixedUint32_t(uint32_t value) {
        writeByte(value & 0xFF);
        writeByte((value >> 8) & 0xFF);
        writeByte((value >> 16) & 0xFF);
        writeByte((value >> 24) & 0xFF);
    }
    void writeFixedUint16_t(uint16_t value) {
        writeByte(value & 0xFF);
        writeByte(value >> 8);
    }
    void writeNativeEndianUint32_t(uint32_t value) {
        
        JS_ASSERT(length() % sizeof(uint32_t) == 0);
        writeFixedUint32_t(0);
        if (oom())
            return;
        uint8_t *endPtr = buffer() + length();
        reinterpret_cast<uint32_t *>(endPtr)[-1] = value;
    }
    size_t length() const {
        return buffer_.length();
    }
    uint8_t *buffer() {
        return &buffer_[0];
    }
    const uint8_t *buffer() const {
        return &buffer_[0];
    }
    bool oom() const {
        return !enoughMemory_;
    }
};

CompactBufferReader::CompactBufferReader(const CompactBufferWriter &writer)
  : buffer_(writer.buffer()),
    end_(writer.buffer() + writer.length())
{
}

} 
} 

#endif 
