





#include "ObexBase.h"

BEGIN_BLUETOOTH_NAMESPACE








int
AppendHeader(uint8_t aHeaderId, uint8_t* aRetBuf, int aBufferSize,
             const uint8_t* aData, int aLength)
{
  int headerLength = aLength + 3;

  aRetBuf[0] = aHeaderId;
  aRetBuf[1] = (headerLength & 0xFF00) >> 8;
  aRetBuf[2] = headerLength & 0x00FF;
  memcpy(&aRetBuf[3], aData, (aLength < aBufferSize - 3) ? aLength
                                                         : aBufferSize - 3);

  return headerLength;
}




int
AppendHeader(uint8_t aHeaderId, uint8_t* aRetBuf, int aValue)
{
  aRetBuf[0] = aHeaderId;
  aRetBuf[1] = (aValue & 0xFF000000) >> 24;
  aRetBuf[2] = (aValue & 0x00FF0000) >> 16;
  aRetBuf[3] = (aValue & 0x0000FF00) >> 8;
  aRetBuf[4] = aValue & 0x000000FF;

  return 5;
}





int
AppendHeaderName(uint8_t* aRetBuf, int aBufferSize, const uint8_t* aName,
                 int aLength)
{
  return AppendHeader(ObexHeaderId::Name, aRetBuf, aBufferSize,
                      aName, aLength);
}

int
AppendHeaderBody(uint8_t* aRetBuf, int aBufferSize, const uint8_t* aBody,
                 int aLength)
{
  return AppendHeader(ObexHeaderId::Body, aRetBuf, aBufferSize,
                      aBody, aLength);
}

int
AppendHeaderWho(uint8_t* aRetBuf, int aBufferSize, const uint8_t* aWho,
                int aLength)
{
  return AppendHeader(ObexHeaderId::Who, aRetBuf, aBufferSize,
                      aWho, aLength);
}

int
AppendHeaderLength(uint8_t* aRetBuf, int aObjectLength)
{
  return AppendHeader(ObexHeaderId::Length, aRetBuf, aObjectLength);
}

int
AppendHeaderConnectionId(uint8_t* aRetBuf, int aConnectionId)
{
  return AppendHeader(ObexHeaderId::ConnectionId, aRetBuf, aConnectionId);
}

int
AppendHeaderEndOfBody(uint8_t* aRetBuf)
{
  aRetBuf[0] = ObexHeaderId::EndOfBody;
  aRetBuf[1] = 0x00;
  aRetBuf[2] = 0x03;

  return 3;
}

void
SetObexPacketInfo(uint8_t* aRetBuf, uint8_t aOpcode, int aPacketLength)
{
  aRetBuf[0] = aOpcode;
  aRetBuf[1] = (aPacketLength & 0xFF00) >> 8;
  aRetBuf[2] = aPacketLength & 0x00FF;
}

bool
ParseHeaders(const uint8_t* aHeaderStart,
             int aTotalLength,
             ObexHeaderSet* aRetHandlerSet)
{
  const uint8_t* ptr = aHeaderStart;

  while (ptr - aHeaderStart < aTotalLength) {
    ObexHeaderId headerId = (ObexHeaderId)*ptr++;

    uint16_t contentLength = 0;
    uint8_t highByte, lowByte;

    
    switch (headerId >> 6)
    {
      case 0x00:
        
        
      case 0x01:
        
        highByte = *ptr++;
        lowByte = *ptr++;
        contentLength = (((uint16_t)highByte << 8) | lowByte) - 3;
        break;

      case 0x02:
        
        contentLength = 1;
        break;

      case 0x03:
        
        contentLength = 4;
        break;
    }

    
    if (ptr + contentLength > aHeaderStart + aTotalLength) {
      
      
      MOZ_ASSERT(false);
      aRetHandlerSet->ClearHeaders();
      return false;
    }

    aRetHandlerSet->AddHeader(new ObexHeader(headerId, contentLength, ptr));
    ptr += contentLength;
  }

  return true;
}

END_BLUETOOTH_NAMESPACE
