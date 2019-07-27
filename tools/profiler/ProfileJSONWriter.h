




#ifndef PROFILEJSONWRITER_H
#define PROFILEJSONWRITER_H

#include <ostream>
#include <string>
#include <string.h>

#include "mozilla/JSONWriter.h"
#include "mozilla/UniquePtr.h"

class SpliceableChunkedJSONWriter;






class ChunkedJSONWriteFunc : public mozilla::JSONWriteFunc
{
public:
  friend class SpliceableJSONWriter;

  ChunkedJSONWriteFunc() {
    AllocChunk(kChunkSize);
  }

  bool IsEmpty() const {
    MOZ_ASSERT_IF(!mChunkPtr, !mChunkEnd &&
                              mChunkList.length() == 0 &&
                              mChunkLengths.length() == 0);
    return !mChunkPtr;
  }

  void Write(const char* aStr) override;
  mozilla::UniquePtr<char[]> CopyData() const;
  void Take(ChunkedJSONWriteFunc&& aOther);

private:
  void AllocChunk(size_t aChunkSize);

  static const size_t kChunkSize = 4096 * 512;

  
  
  
  
  char* mChunkPtr;

  
  
  
  
  char* mChunkEnd;

  
  
  
  
  mozilla::Vector<mozilla::UniquePtr<char[]>> mChunkList;
  mozilla::Vector<size_t> mChunkLengths;
};

struct OStreamJSONWriteFunc : public mozilla::JSONWriteFunc
{
  explicit OStreamJSONWriteFunc(std::ostream& aStream)
    : mStream(aStream)
  { }

  void Write(const char* aStr) override {
    mStream << aStr;
  }

  std::ostream& mStream;
};

class SpliceableJSONWriter : public mozilla::JSONWriter
{
public:
  explicit SpliceableJSONWriter(mozilla::UniquePtr<mozilla::JSONWriteFunc> aWriter)
    : JSONWriter(mozilla::Move(aWriter))
  { }

  void StartBareList(CollectionStyle aStyle = SingleLineStyle) {
    StartCollection(nullptr, "", aStyle);
  }

  void EndBareList() {
    EndCollection("");
  }

  void NullElements(uint32_t aCount) {
    for (uint32_t i = 0; i < aCount; i++) {
      NullElement();
    }
  }

  void Splice(const ChunkedJSONWriteFunc* aFunc);
  void Splice(const char* aStr);

  
  
  
  virtual void TakeAndSplice(ChunkedJSONWriteFunc* aFunc);
};

class SpliceableChunkedJSONWriter : public SpliceableJSONWriter
{
public:
  explicit SpliceableChunkedJSONWriter()
    : SpliceableJSONWriter(mozilla::MakeUnique<ChunkedJSONWriteFunc>())
  { }

  ChunkedJSONWriteFunc* WriteFunc() const {
    return static_cast<ChunkedJSONWriteFunc*>(JSONWriter::WriteFunc());
  }

  
  virtual void TakeAndSplice(ChunkedJSONWriteFunc* aFunc) override;
};

#endif 
