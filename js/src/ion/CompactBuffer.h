








































#ifndef jsion_compact_buffer_h__
#define jsion_compact_buffer_h__

#include "jsvector.h"

namespace js {
namespace ion {










class CompactBufferReader
{
    uint8 *buffer_;
    uint8 *end_;

    template <typename T> T readVariableLength() {
        JS_ASSERT(sizeof(T) == 4);
        T val = 0;
        uint32 shift = 0;
        uint byte;
        while (true) {
            JS_ASSERT(shift < 32);
            byte = readByte();
            val |= (uint32(byte) >> 1) << shift;
            shift += 7;
            if (!(byte & 1)) {
                if (shift < 32) {
                    
                    
                    val <<= 32 - shift;
                    val >>= 32 - shift;
                }
                return val;
            }
        }
        JS_NOT_REACHED("unreachable");
        return 0;
    }

  public:
    CompactBufferReader(uint8 *start, uint8 *end) : buffer_(start), end_(end)
    { }
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
        return readVariableLength<uint32>();
    }
    int32 readSigned() {
        return readVariableLength<int32>();
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
    void writeSigned(int32 value) {
        do {
            int32 shifted = value >> 7;
            uint8 byte = ((value & 0x7F) << 1) | (shifted != 0 && shifted != -1);
            writeByte(byte);
            value = shifted;
        } while (value != 0 && value != -1);
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
    bool outOfMemory() const {
        return !enoughMemory_;
    }
};

} 
} 

#endif 

