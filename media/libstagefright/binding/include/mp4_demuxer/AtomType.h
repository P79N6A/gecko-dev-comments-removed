





#ifndef ATOM_TYPE_H_
#define ATOM_TYPE_H_

#include <stdint.h>
#include "mozilla/Endian.h"

using namespace mozilla;

namespace mp4_demuxer {

class AtomType
{
public:
  AtomType() : mType(0) { }
  AtomType(uint32_t aType) : mType(aType) { }
  AtomType(const char* aType) : mType(BigEndian::readUint32(aType)) { }
  bool operator==(const AtomType& aType) const { return mType == aType.mType; }

private:
  uint32_t mType;
};
}

#endif
