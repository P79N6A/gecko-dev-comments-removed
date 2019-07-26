









#include "bitstream_parser.h"

namespace webrtc {
BitstreamParser::BitstreamParser(const uint8_t* data, const uint32_t dataLength) :
    _data(data),
    _dataLength(dataLength),
    _byteOffset(0),
    _bitOffset(0)
{
}
    

uint8_t
BitstreamParser::Get1Bit()
{
    uint8_t retVal = 0x1 & (_data[_byteOffset] >> (7-_bitOffset++));

    
    if(_bitOffset == 8)
    {
        _bitOffset = 0;
        _byteOffset++;
    }
    return retVal;
}

uint8_t
BitstreamParser::Get2Bits()
{
    uint8_t retVal = (Get1Bit() << 1);
    retVal += Get1Bit();
    return retVal;
}

uint8_t
BitstreamParser::Get3Bits()
{
    uint8_t retVal = (Get1Bit() << 2);
    retVal += (Get1Bit() << 1);
    retVal += Get1Bit();
    return retVal;
}

uint8_t
BitstreamParser::Get4Bits()
{
    uint8_t retVal = (Get1Bit() << 3);
    retVal += (Get1Bit() << 2);
    retVal += (Get1Bit() << 1);
    retVal += Get1Bit();
    return retVal;
}

uint8_t
BitstreamParser::Get5Bits()
{
    uint8_t retVal = (Get1Bit() << 4);
    retVal += (Get1Bit() << 3);
    retVal += (Get1Bit() << 2);
    retVal += (Get1Bit() << 1);
    retVal += Get1Bit();
    return retVal;
}

uint8_t
BitstreamParser::Get6Bits()
{
    uint8_t retVal = (Get1Bit() << 5);
    retVal += (Get1Bit() << 4);
    retVal += (Get1Bit() << 3);
    retVal += (Get1Bit() << 2);
    retVal += (Get1Bit() << 1);
    retVal += Get1Bit();
    return retVal;
}

uint8_t
BitstreamParser::Get7Bits()
{
    uint8_t retVal = (Get1Bit() << 6);
    retVal += (Get1Bit() << 5);
    retVal += (Get1Bit() << 4);
    retVal += (Get1Bit() << 3);
    retVal += (Get1Bit() << 2);
    retVal += (Get1Bit() << 1);
    retVal += Get1Bit();
    return retVal;
}

uint8_t
BitstreamParser::Get8Bits()
{
    uint16_t retVal;

    if(_bitOffset != 0)
    {
        
        retVal = (_data[_byteOffset] << 8)+ (_data[_byteOffset+1]) ;
        retVal = retVal >> (8-_bitOffset);
    } else
    {
        retVal = _data[_byteOffset];
    }
    _byteOffset++;
    return (uint8_t)retVal;
}

uint16_t
BitstreamParser::Get16Bits()
{
    uint32_t retVal;

    if(_bitOffset != 0)
    {
        
        retVal = (_data[_byteOffset] << 16) + (_data[_byteOffset+1] << 8) + (_data[_byteOffset+2]);
        retVal = retVal >> (8-_bitOffset);
    }else
    {
        
        retVal = (_data[_byteOffset] << 8) + (_data[_byteOffset+1]) ;
    }
    _byteOffset += 2;
    return (uint16_t)retVal;
}

uint32_t
BitstreamParser::Get24Bits()
{
    uint32_t retVal;

    if(_bitOffset != 0)
    {
        
        retVal = (_data[_byteOffset] << 24) + (_data[_byteOffset+1] << 16) + (_data[_byteOffset+2] << 8) + (_data[_byteOffset+3]);
        retVal = retVal >> (8-_bitOffset);
    }else
    {
        
        retVal = (_data[_byteOffset] << 16) + (_data[_byteOffset+1] << 8) + (_data[_byteOffset+2]) ;
    }
    _byteOffset += 3;
    return retVal & 0x00ffffff; 
}

uint32_t
BitstreamParser::Get32Bits()
{
    uint32_t retVal;

    if(_bitOffset != 0)
    {
        
        uint64_t tempVal = _data[_byteOffset];
        tempVal <<= 8;
        tempVal += _data[_byteOffset+1];
        tempVal <<= 8;
        tempVal += _data[_byteOffset+2];
        tempVal <<= 8;
        tempVal += _data[_byteOffset+3];
        tempVal <<= 8;
        tempVal += _data[_byteOffset+4];
        tempVal >>= (8-_bitOffset);

        retVal = uint32_t(tempVal);
    }else
    {
        
        retVal = (_data[_byteOffset]<< 24) + (_data[_byteOffset+1] << 16) + (_data[_byteOffset+2] << 8) + (_data[_byteOffset+3]) ;
    }
    _byteOffset += 4;
    return retVal;
}













uint32_t
BitstreamParser::GetUE()
{
    uint32_t retVal = 0;
    uint8_t numLeadingZeros = 0;

    while (Get1Bit() != 1)
    {
        numLeadingZeros++;
    }
    
    retVal = (1 << numLeadingZeros) - 1;

    
    while (numLeadingZeros)
    {
        retVal += (Get1Bit() << --numLeadingZeros);
    }
    return retVal;
}
}  
