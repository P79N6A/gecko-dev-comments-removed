








































#ifndef jsion_compact_buffer_h__
#define jsion_compact_buffer_h__

#include "jsvector.h"

namespace js {
namespace ion {

class CompactBufferWriter;










class CompactBufferReader
{
    const uint8 *buffer_;
    const uint8 *end_;

    uint32 readVariableLength() {
        uint32 val = 0;
        uint32 shift = 0;
        uint byte;
        while (true) {
            JS_ASSERT(shift < 32);
            byte = readByte();
            val |= (uint32(byte) >> 1) << shift;
            shift += 7;
            if (!(byte & 1))
                return val;
        }
        JS_NOT_REACHED("unreachable");
        return 0;
    }

  public:
    CompactBufferReader(const uint8 *start, const uint8 *end)
      : buffer_(start),
        end_(end)
    { }
    inline CompactBufferReader(const CompactBufferWriter &writer);
    uint8 readByte() {
        JS_ASSERT(buffer_ < end_);
        return *buffer_++;
    }
    uint32 readFixedUint32() {
        uint32 b0 = readByte();
        uint32 b1 = readByte();
        uint32 b2 = readByte();
        uint32 b3 = readByte();
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }
    uint32 readUnsigned() {
        return readVariableLength();
    }
    int32 readSigned() {
        uint8 b = readByte();
        bool isNegative = !!(b & (1 << 0));
        bool more = !!(b & (1 << 1));
        int32 result = b >> 2;
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
};

class CompactBufferWriter
{
    js::Vector<uint8, 32, SystemAllocPolicy> buffer_;
    bool enoughMemory_;

  public:
    CompactBufferWriter()
      : enoughMemory_(true)
    { }

    
    
    void writeByte(uint32 byte) {
        JS_ASSERT(byte <= 0xFF);
        enoughMemory_ &= buffer_.append(byte);
    }
    void writeUnsigned(uint32 value) {
        do {
            uint8 byte = ((value & 0x7F) << 1) | (value > 0x7F);
            writeByte(byte);
            value >>= 7;
        } while (value);
    }
    void writeSigned(int32 v) {
        bool isNegative = v < 0;
        uint32 value = isNegative ? -v : v;
        uint8 byte = ((value & 0x3F) << 2) | ((value > 0x3F) << 1) | uint32(isNegative);
        writeByte(byte);

        
        value >>= 6;
        if (value == 0)
            return;
        writeUnsigned(value);
    }
    void writeFixedUint32(uint32 value) {
        writeByte(value & 0xFF);
        writeByte((value >> 8) & 0xFF);
        writeByte((value >> 16) & 0xFF);
        writeByte((value >> 24) & 0xFF);
    }
    size_t length() const {
        return buffer_.length();
    }
    uint8 *buffer() {
        return &buffer_[0];
    }
    const uint8 *buffer() const {
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

