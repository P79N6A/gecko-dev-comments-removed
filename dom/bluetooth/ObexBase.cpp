





#include "ObexBase.h"

BEGIN_BLUETOOTH_NAMESPACE

int
AppendHeaderName(uint8_t* aRetBuf, const char* aName, int aLength)
{
  int headerLength = aLength + 3;

  aRetBuf[0] = ObexHeaderId::Name;
  aRetBuf[1] = (headerLength & 0xFF00) >> 8;
  aRetBuf[2] = headerLength & 0x00FF;

  memcpy(&aRetBuf[3], aName, aLength);

  return headerLength;
}

int
AppendHeaderBody(uint8_t* aRetBuf, uint8_t* aData, int aLength)
{
  int headerLength = aLength + 3;

  aRetBuf[0] = ObexHeaderId::Body;
  aRetBuf[1] = (headerLength & 0xFF00) >> 8;
  aRetBuf[2] = headerLength & 0x00FF;

  memcpy(&aRetBuf[3], aData, aLength);

  return headerLength;
}

int
AppendHeaderLength(uint8_t* aRetBuf, int aObjectLength)
{
  aRetBuf[0] = ObexHeaderId::Length;
  aRetBuf[1] = (aObjectLength & 0xFF000000) >> 24;
  aRetBuf[2] = (aObjectLength & 0x00FF0000) >> 16;
  aRetBuf[3] = (aObjectLength & 0x0000FF00) >> 8;
  aRetBuf[4] = aObjectLength & 0x000000FF;

  return 5;
}

int
AppendHeaderConnectionId(uint8_t* aRetBuf, int aConnectionId)
{
  aRetBuf[0] = ObexHeaderId::ConnectionId;
  aRetBuf[1] = (aConnectionId & 0xFF000000) >> 24;
  aRetBuf[2] = (aConnectionId & 0x00FF0000) >> 16;
  aRetBuf[3] = (aConnectionId & 0x0000FF00) >> 8;
  aRetBuf[4] = aConnectionId & 0x000000FF;

  return 5;
}

void
SetObexPacketInfo(uint8_t* aRetBuf, uint8_t aOpcode, int aPacketLength)
{
  aRetBuf[0] = aOpcode;
  aRetBuf[1] = (aPacketLength & 0xFF00) >> 8;
  aRetBuf[2] = aPacketLength & 0x00FF;
}

int
ParseHeadersAndFindBody(uint8_t* aHeaderStart,
                        int aTotalLength,
                        ObexHeaderSet* aRetHandlerSet)
{
  uint8_t* ptr = aHeaderStart;

  while (ptr - aHeaderStart < aTotalLength) {
    ObexHeaderId headerId = (ObexHeaderId)*ptr;

    if (headerId == ObexHeaderId::Body ||
        headerId == ObexHeaderId::EndOfBody) {
      return ptr - aHeaderStart;
    }

    ++ptr;

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

    uint8_t* content = new uint8_t[contentLength];
    memcpy(content, ptr, contentLength);
    aRetHandlerSet->AddHeader(new ObexHeader(headerId, contentLength, content));

    ptr += contentLength;
  }

  return -1;
}

END_BLUETOOTH_NAMESPACE
