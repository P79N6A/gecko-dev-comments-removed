









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_BYTE_IO_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_BYTE_IO_H_


























#include <limits>

#include "webrtc/typedefs.h"

namespace webrtc {




template<typename T, unsigned int B = sizeof(T),
    bool is_signed = std::numeric_limits<T>::is_signed>
class ByteReader {
 public:
  static T ReadBigEndian(uint8_t* data) {
    if (is_signed && B < sizeof(T)) {
      return SignExtend(InternalReadBigEndian(data));
    }
    return InternalReadBigEndian(data);
  }

  static T ReadLittleEndian(uint8_t* data) {
    if (is_signed && B < sizeof(T)) {
      return SignExtend(InternalReadLittleEndian(data));
    }
    return InternalReadLittleEndian(data);
  }

 private:
  static T InternalReadBigEndian(uint8_t* data) {
    T val(0);
    for (unsigned int i = 0; i < B; ++i) {
      val |= static_cast<T>(data[i]) << ((B - 1 - i) * 8);
    }
    return val;
  }

  static T InternalReadLittleEndian(uint8_t* data) {
    T val(0);
    for (unsigned int i = 0; i < B; ++i) {
      val |= static_cast<T>(data[i]) << (i * 8);
    }
    return val;
  }

  
  
  
  
  
  static T SignExtend(T val) {
    uint8_t msb = static_cast<uint8_t>(val >> ((B - 1) * 8));
    if (msb & 0x80) {
      
      
      
      T sign_extend = (sizeof(T) == B ? 0 :
          (static_cast<T>(-1L) << ((B % sizeof(T)) * 8)));

      return val | sign_extend;
    }
    return val;
  }
};



template<typename T, unsigned int B = sizeof(T)>
class ByteWriter {
 public:
  static void WriteBigEndian(uint8_t* data, T val) {
    for (unsigned int i = 0; i < B; ++i) {
      data[i] = val >> ((B - 1 - i) * 8);
    }
  }

  static void WriteLittleEndian(uint8_t* data, T val) {
    for (unsigned int i = 0; i < B; ++i) {
      data[i] = val >> (i * 8);
    }
  }
};






template<typename T, bool is_signed>
class ByteReader<T, 2, is_signed> {
 public:
  static T ReadBigEndian(uint8_t* data) {
    return (data[0] << 8) | data[1];
  }

  static T ReadLittleEndian(uint8_t* data) {
    return data[0] | (data[1] << 8);
  }
};

template<typename T>
class ByteWriter<T, 2> {
 public:
  static void WriteBigEndian(uint8_t* data, T val) {
    data[0] = val >> 8;
    data[1] = val;
  }

  static void WriteLittleEndian(uint8_t* data, T val) {
    data[0] = val;
    data[1] = val >> 8;
  }
};


template<typename T, bool is_signed>
class ByteReader<T, 4, is_signed> {
 public:
  static T ReadBigEndian(uint8_t* data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
  }

  static T ReadLittleEndian(uint8_t* data) {
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  }
};


template<typename T>
class ByteWriter<T, 4> {
 public:
  static void WriteBigEndian(uint8_t* data, T val) {
    data[0] = val >> 24;
    data[1] = val >> 16;
    data[2] = val >> 8;
    data[3] = val;
  }

  static void WriteLittleEndian(uint8_t* data, T val) {
    data[0] = val;
    data[1] = val >> 8;
    data[2] = val >> 16;
    data[3] = val >> 24;
  }
};


template<typename T, bool is_signed>
class ByteReader<T, 8, is_signed> {
 public:
  static T ReadBigEndian(uint8_t* data) {
    return
        (Get(data, 0) << 56) | (Get(data, 1) << 48) |
        (Get(data, 2) << 40) | (Get(data, 3) << 32) |
        (Get(data, 4) << 24) | (Get(data, 5) << 16) |
        (Get(data, 6) << 8)  |  Get(data, 7);
  }

  static T ReadLittleEndian(uint8_t* data) {
    return
         Get(data, 0)        | (Get(data, 1) << 8)  |
        (Get(data, 2) << 16) | (Get(data, 3) << 24) |
        (Get(data, 4) << 32) | (Get(data, 5) << 40) |
        (Get(data, 6) << 48) | (Get(data, 7) << 56);
  }

 private:
  inline static T Get(uint8_t* data, unsigned int index) {
    return static_cast<T>(data[index]);
  }
};

template<typename T>
class ByteWriter<T, 8> {
 public:
  static void WriteBigEndian(uint8_t* data, T val) {
    data[0] = val >> 56;
    data[1] = val >> 48;
    data[2] = val >> 40;
    data[3] = val >> 32;
    data[4] = val >> 24;
    data[5] = val >> 16;
    data[6] = val >> 8;
    data[7] = val;
  }

  static void WriteLittleEndian(uint8_t* data, T val) {
    data[0] = val;
    data[1] = val >> 8;
    data[2] = val >> 16;
    data[3] = val >> 24;
    data[4] = val >> 32;
    data[5] = val >> 40;
    data[6] = val >> 48;
    data[7] = val >> 56;
  }
};

}  

#endif  
