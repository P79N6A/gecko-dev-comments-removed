




#include "EXIF.h"

#include "mozilla/Endian.h"

namespace mozilla {
namespace image {







enum EXIFTag
{
  OrientationTag = 0x112,
};


enum EXIFType
{
  ByteType       = 1,
  ASCIIType      = 2,
  ShortType      = 3,
  LongType       = 4,
  RationalType   = 5,
  UndefinedType  = 7,
  SignedLongType = 9,
  SignedRational = 10,
};

static const char* EXIFHeader = "Exif\0\0";
static const uint32_t EXIFHeaderLength = 6;




EXIFData
EXIFParser::ParseEXIF(const uint8_t* aData, const uint32_t aLength)
{
  if (!Initialize(aData, aLength))
    return EXIFData();

  if (!ParseEXIFHeader())
    return EXIFData();

  uint32_t offsetIFD;
  if (!ParseTIFFHeader(offsetIFD))
    return EXIFData();

  JumpTo(offsetIFD);

  Orientation orientation;
  if (!ParseIFD0(orientation))
    return EXIFData();

  
  
  return EXIFData(orientation);
}




bool
EXIFParser::ParseEXIFHeader()
{
  return MatchString(EXIFHeader, EXIFHeaderLength);
}




bool
EXIFParser::ParseTIFFHeader(uint32_t& aIFD0OffsetOut)
{
  
  if (MatchString("MM\0*", 4))
    mByteOrder = ByteOrder::BigEndian;
  else if (MatchString("II*\0", 4))
    mByteOrder = ByteOrder::LittleEndian;
  else
    return false;

  
  
  uint32_t ifd0Offset;
  if (!ReadUInt32(ifd0Offset) || ifd0Offset > 64 * 1024)
    return false;

  
  
  
  aIFD0OffsetOut = ifd0Offset + EXIFHeaderLength;
  return true;
}




bool
EXIFParser::ParseIFD0(Orientation& aOrientationOut)
{
  uint16_t entryCount;
  if (!ReadUInt16(entryCount))
    return false;

  for (uint16_t entry = 0 ; entry < entryCount ; ++entry) {
    
    uint16_t tag;
    if (!ReadUInt16(tag))
      return false;

    
    
    if (tag != OrientationTag) {
      Advance(10);
      continue;
    }

    uint16_t type;
    if (!ReadUInt16(type))
      return false;

    uint32_t count;
    if (!ReadUInt32(count))
      return false;

    
    if (!ParseOrientation(type, count, aOrientationOut))
      return false;

    
    return true;
  }

  
  
  aOrientationOut = Orientation();
  return true;
}

bool
EXIFParser::ParseOrientation(uint16_t aType, uint32_t aCount, Orientation& aOut)
{
  
  if (aType != ShortType || aCount != 1)
    return false;

  uint16_t value;
  if (!ReadUInt16(value))
    return false;

  switch (value) {
    case 1: aOut = Orientation(Angle::D0,   Flip::Unflipped);  break;
    case 2: aOut = Orientation(Angle::D0,   Flip::Horizontal); break;
    case 3: aOut = Orientation(Angle::D180, Flip::Unflipped);  break;
    case 4: aOut = Orientation(Angle::D180, Flip::Horizontal); break;
    case 5: aOut = Orientation(Angle::D90,  Flip::Horizontal); break;
    case 6: aOut = Orientation(Angle::D90,  Flip::Unflipped);  break;
    case 7: aOut = Orientation(Angle::D270, Flip::Horizontal); break;
    case 8: aOut = Orientation(Angle::D270, Flip::Unflipped);  break;
    default: return false;
  }

  
  
  Advance(2);
  return true;
}

bool
EXIFParser::Initialize(const uint8_t* aData, const uint32_t aLength)
{
  if (aData == nullptr)
    return false;

  
  if (aLength > 64 * 1024)
    return false;

  mStart = mCurrent = aData;
  mLength = mRemainingLength = aLength;
  mByteOrder = ByteOrder::Unknown;
  return true;
}

void
EXIFParser::Advance(const uint32_t aDistance)
{
  if (mRemainingLength >= aDistance) {
    mCurrent += aDistance;
    mRemainingLength -= aDistance;
  } else {
    mCurrent = mStart;
    mRemainingLength = 0;
  }
}

void
EXIFParser::JumpTo(const uint32_t aOffset)
{
  if (mLength >= aOffset) {
    mCurrent = mStart + aOffset;
    mRemainingLength = mLength - aOffset;
  } else {
    mCurrent = mStart;
    mRemainingLength = 0;
  }
}

bool
EXIFParser::MatchString(const char* aString, const uint32_t aLength)
{
  if (mRemainingLength < aLength)
    return false;

  for (uint32_t i = 0 ; i < aLength ; ++i) {
    if (mCurrent[i] != aString[i])
      return false;
  }

  Advance(aLength);
  return true;
}

bool
EXIFParser::MatchUInt16(const uint16_t aValue)
{
  if (mRemainingLength < 2)
    return false;
  
  bool matched;
  switch (mByteOrder) {
    case ByteOrder::LittleEndian:
      matched = LittleEndian::readUint16(mCurrent) == aValue;
      break;
    case ByteOrder::BigEndian:
      matched = BigEndian::readUint16(mCurrent) == aValue;
      break;
    default:
      NS_NOTREACHED("Should know the byte order by now");
      matched = false;
  }

  if (matched)
    Advance(2);

  return matched;
}

bool
EXIFParser::ReadUInt16(uint16_t& aValue)
{
  if (mRemainingLength < 2)
    return false;
  
  bool matched = true;
  switch (mByteOrder) {
    case ByteOrder::LittleEndian:
      aValue = LittleEndian::readUint16(mCurrent);
      break;
    case ByteOrder::BigEndian:
      aValue = BigEndian::readUint16(mCurrent);
      break;
    default:
      NS_NOTREACHED("Should know the byte order by now");
      matched = false;
  }

  if (matched)
    Advance(2);

  return matched;
}

bool
EXIFParser::ReadUInt32(uint32_t& aValue)
{
  if (mRemainingLength < 4)
    return false;
  
  bool matched = true;
  switch (mByteOrder) {
    case ByteOrder::LittleEndian:
      aValue = LittleEndian::readUint32(mCurrent);
      break;
    case ByteOrder::BigEndian:
      aValue = BigEndian::readUint32(mCurrent);
      break;
    default:
      NS_NOTREACHED("Should know the byte order by now");
      matched = false;
  }

  if (matched)
    Advance(4);

  return matched;
}

} 
} 
