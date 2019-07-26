





#include "ObexBase.h"

BEGIN_BLUETOOTH_NAMESPACE

int
AppendHeaderName(uint8_t* retBuf, const char* name, int length)
{
  int headerLength = length + 3;

  retBuf[0] = ObexHeaderId::Name;
  retBuf[1] = (headerLength & 0xFF00) >> 8;
  retBuf[2] = headerLength & 0x00FF;

  memcpy(&retBuf[3], name, length);

  return headerLength;
}

int
AppendHeaderBody(uint8_t* retBuf, uint8_t* data, int length)
{
  int headerLength = length + 3;

  retBuf[0] = ObexHeaderId::Body;
  retBuf[1] = (headerLength & 0xFF00) >> 8;
  retBuf[2] = headerLength & 0x00FF;

  memcpy(&retBuf[3], data, length);

  return headerLength;
}

int
AppendHeaderLength(uint8_t* retBuf, int objectLength)
{
  retBuf[0] = ObexHeaderId::Length;
  retBuf[1] = (objectLength & 0xFF000000) >> 24;
  retBuf[2] = (objectLength & 0x00FF0000) >> 16;
  retBuf[3] = (objectLength & 0x0000FF00) >> 8;
  retBuf[4] = objectLength & 0x000000FF;

  return 5;
}

int
AppendHeaderConnectionId(uint8_t* retBuf, int connectionId)
{
  retBuf[0] = ObexHeaderId::ConnectionId;
  retBuf[1] = (connectionId & 0xFF000000) >> 24;
  retBuf[2] = (connectionId & 0x00FF0000) >> 16;
  retBuf[3] = (connectionId & 0x0000FF00) >> 8;
  retBuf[4] = connectionId & 0x000000FF;

  return 5;
}

void
SetObexPacketInfo(uint8_t* retBuf, uint8_t opcode, int packetLength)
{
  retBuf[0] = opcode;
  retBuf[1] = (packetLength & 0xFF00) >> 8;
  retBuf[2] = packetLength & 0x00FF;
}

void
ParseHeaders(uint8_t* buf, int totalLength, ObexHeaderSet* retHandlerSet)
{
  uint8_t* ptr = buf;

  while (ptr - buf < totalLength) {
    ObexHeaderId headerId = (ObexHeaderId)*ptr++;
    int contentLength = 0;
    uint8_t highByte, lowByte;

    
    switch (headerId >> 6)
    {
      case 0x00:
        
      case 0x01:
        
        highByte = *ptr++;
        lowByte = *ptr++;
        contentLength = (((int)highByte << 8) | lowByte) - 3;
        break;

      case 0x02:
        
        contentLength = 1;
        break;

      case 0x03:
        
        contentLength = 4;
        break;
    }

    
    
    if (contentLength + (ptr - buf) > totalLength) {
      break;
    }

    uint8_t* content = new uint8_t[contentLength];
    memcpy(content, ptr, contentLength);
    retHandlerSet->AddHeader(new ObexHeader(headerId, contentLength, content));

    ptr += contentLength;
  }
}

END_BLUETOOTH_NAMESPACE
