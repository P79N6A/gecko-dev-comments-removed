




#include "mozilla/HashFunctions.h"

#include "ProfileJSONWriter.h"

void
SpliceableJSONWriter::Splice(const ChunkedJSONWriteFunc* aFunc)
{
  Separator();
  for (size_t i = 0; i < aFunc->mChunkList.length(); i++) {
    WriteFunc()->Write(aFunc->mChunkList[i].get());
  }
  mNeedComma[mDepth] = true;
}

void
SpliceableJSONWriter::Splice(const char* aStr)
{
  Separator();
  WriteFunc()->Write(aStr);
  mNeedComma[mDepth] = true;
}
