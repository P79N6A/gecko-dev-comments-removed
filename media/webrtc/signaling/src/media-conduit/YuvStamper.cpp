



#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#elif defined XP_WIN
#include <winsock2.h>
#endif

#include "YuvStamper.h"

typedef uint32_t UINT4; 
extern "C" {
#include "r_crc32.h"
}

namespace mozilla {

  YuvStamper::YuvStamper(uint8_t* pYData,
			 uint32_t width,
			 uint32_t height,
			 uint32_t stride,
			 uint32_t x,
			 uint32_t y):
    pYData(pYData), mStride(stride),
    mWidth(width), mHeight(height),
    mCursor(x, y) {}

  bool YuvStamper::Encode(uint32_t width, uint32_t height, uint32_t stride,
			  uint8_t* pYData, uint8_t* pMsg, size_t msg_len,
			  uint32_t x, uint32_t y)
  {
    YuvStamper stamper(pYData, width, height, stride, x, y);

    
    if (stamper.Capacity() < 8 * (msg_len + sizeof(uint32_t)))
    {
      return false;
    }

    bool ok = false;
    uint32_t crc;
    uint8_t* pCrc = reinterpret_cast<uint8_t*>(&crc);
    r_crc32(reinterpret_cast<char*>(pMsg), (int)msg_len, &crc);
    crc = htonl(crc);

    while (msg_len-- > 0) {
      if (!stamper.Write8(*pMsg++)) {
	return false;
      }
    }

    
    ok = stamper.Write8(*pCrc++) &&
         stamper.Write8(*pCrc++) &&
         stamper.Write8(*pCrc++) &&
         stamper.Write8(*pCrc++);

    return ok;
  }

  bool YuvStamper::Decode(uint32_t width, uint32_t height, uint32_t stride,
			  uint8_t* pYData, uint8_t* pMsg, size_t msg_len,
			  uint32_t x, uint32_t y)
  {
    YuvStamper stamper(pYData, width, height, stride, x, y);
    uint8_t* ptr = pMsg;
    size_t len = msg_len;
    uint32_t crc, msg_crc;
    uint8_t* pCrc = reinterpret_cast<uint8_t*>(&crc);

    
    if (stamper.Capacity() < 8 * (len + sizeof(uint32_t))) {
      return false;
    }

    while (len-- > 0) {
      if(!stamper.Read8(*ptr++)) {
	return false;
      }
    }

    if (!(stamper.Read8(*pCrc++) &&
          stamper.Read8(*pCrc++) &&
          stamper.Read8(*pCrc++) &&
          stamper.Read8(*pCrc++))) {
      return false;
    }

    r_crc32(reinterpret_cast<char*>(pMsg), (int)msg_len, &msg_crc);
    return crc == htonl(msg_crc);
  }

  inline uint32_t YuvStamper::Capacity()
  {
    
    if (mCursor.y + sBitSize > mHeight) {
      return 0;
    }

    if (mCursor.x + sBitSize > mWidth && !AdvanceCursor()) {
      return 0;
    }

    
    uint32_t width = mWidth / sBitSize;
    uint32_t height = mHeight / sBitSize;
    uint32_t x = mCursor.x / sBitSize;
    uint32_t y = mCursor.y / sBitSize;

    return (width * height - width * y)- x;
  }

  bool YuvStamper::Write8(uint8_t value)
  {
    
    uint8_t mask = 0x80;
    while (mask) {
      if (!WriteBit(!!(value & mask))) {
	return false;
      }
      mask >>= 1;
    }
    return true;
  }

  bool YuvStamper::WriteBit(bool one)
  {
    
    uint8_t value = one ? sYOn : sYOff;
    for (uint32_t y = 0; y < sBitSize; y++) {
      for (uint32_t x = 0; x < sBitSize; x++) {
	*(pYData + (mCursor.x + x) + ((mCursor.y + y) * mStride)) = value;
      }
    }

    return AdvanceCursor();
  }

  bool YuvStamper::AdvanceCursor()
  {
    mCursor.x += sBitSize;
    if (mCursor.x + sBitSize > mWidth) {
      
      mCursor.y += sBitSize;
      if (mCursor.y + sBitSize > mHeight) {
	
	mCursor.y -= sBitSize;
	mCursor.x -= sBitSize;
	return false;
      } else {
	mCursor.x = 0;
      }
    }

    return true;
  }

  bool YuvStamper::Read8(uint8_t &value) {
    uint8_t octet = 0;
    uint8_t bit = 0;

    for (int i = 8; i > 0; --i) {
      if (!ReadBit(bit)) {
	return false;
      }
      octet <<= 1;
      octet |= bit;
    }

    value = octet;
    return true;
  }

  bool YuvStamper::ReadBit(uint8_t &bit)
  {
    uint32_t sum = 0;
    for (uint32_t y = 0; y < sBitSize; y++) {
      for (uint32_t x = 0; x < sBitSize; x++) {
	sum += *(pYData + mStride * (mCursor.y + y) + mCursor.x + x);
      }
    }

    
    bit = (sum > (sBitThreshold * sBitSize * sBitSize)) ? 1 : 0;
    return AdvanceCursor();
  }

}  
