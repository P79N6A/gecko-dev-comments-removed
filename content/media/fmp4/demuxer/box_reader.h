



#ifndef MEDIA_MP4_BOX_READER_H_
#define MEDIA_MP4_BOX_READER_H_

#include <map>
#include <vector>

#include "mp4_demuxer/basictypes.h"
#include "mp4_demuxer/fourccs.h"

#include "nsAutoPtr.h"

namespace mp4_demuxer {

class BoxReader;
class Stream;

struct Box {
  virtual ~Box();
  virtual bool Parse(BoxReader* reader) = 0;
  virtual FourCC BoxType() const = 0;
};

class StreamReader {
 public:
  StreamReader(Stream* stream, int64_t offset, int64_t size)
    : start_(offset), size_(size), pos_(0), stream_(stream) {}

  bool HasBytes(int count) { return (pos() + count <= size()); }

  
  
  bool Read1(uint8_t* v)  WARN_UNUSED_RESULT;
  bool Read2(uint16_t* v) WARN_UNUSED_RESULT;
  bool Read2s(int16_t* v) WARN_UNUSED_RESULT;
  bool Read4(uint32_t* v) WARN_UNUSED_RESULT;
  bool Read4s(int32_t* v) WARN_UNUSED_RESULT;
  bool Read8(uint64_t* v) WARN_UNUSED_RESULT;
  bool Read8s(int64_t* v) WARN_UNUSED_RESULT;

  bool ReadFourCC(FourCC* v) WARN_UNUSED_RESULT;

  bool ReadVec(std::vector<uint8_t>* t, int count) WARN_UNUSED_RESULT;

  
  
  bool Read4Into8(uint64_t* v) WARN_UNUSED_RESULT;
  bool Read4sInto8s(int64_t* v) WARN_UNUSED_RESULT;

  
  bool SkipBytes(int nbytes) WARN_UNUSED_RESULT;

  
  int64_t size() const;
  int64_t pos() const;

protected:

  
  const int64_t start_;

  
  int64_t size_;

  
  
  int64_t pos_;

  
  Stream* stream_;

  template<typename T> bool Read(T* t) WARN_UNUSED_RESULT;
};

class BoxReader : public StreamReader {
 public:
  ~BoxReader();

  
  
  
  
  
  
  static BoxReader* ReadTopLevelBox(Stream* stream,
                                    int64_t offset,
                                    bool* err);

  
  
  
  static bool StartTopLevelBox(Stream* stream,
                               int64_t offset,
                               FourCC* type,
                               int* box_size);

  
  
  
  static bool IsValidTopLevelBox(const FourCC& type);

  
  
  bool ScanChildren() WARN_UNUSED_RESULT;

  
  
  bool ReadChild(Box* child) WARN_UNUSED_RESULT;

  
  
  bool MaybeReadChild(Box* child) WARN_UNUSED_RESULT;

  
  template<typename T> bool ReadChildren(
      std::vector<T>* children) WARN_UNUSED_RESULT;

  
  template<typename T> bool MaybeReadChildren(
      std::vector<T>* children) WARN_UNUSED_RESULT;

  
  
  
  template<typename T> bool ReadAllChildren(
      std::vector<T>* children) WARN_UNUSED_RESULT;

  
  
  
  bool ReadFullBoxHeader() WARN_UNUSED_RESULT;

  FourCC type() const   { return type_; }
  uint8_t version() const { return version_; }
  uint32_t flags() const  { return flags_; }

private:

  BoxReader(Stream* stream, int64_t offset, int64_t size);

  
  
  
  
  
  
  
  bool ReadHeader(bool* err);

  FourCC type_;
  uint8_t version_;
  uint32_t flags_;

  typedef std::multimap<FourCC, BoxReader> ChildMap;

  
  
  ChildMap children_;
  bool scanned_;
};


template<typename T> bool BoxReader::ReadChildren(std::vector<T>* children) {
  RCHECK(MaybeReadChildren(children) && !children->empty());
  return true;
}

template<typename T>
bool BoxReader::MaybeReadChildren(std::vector<T>* children) {
  DCHECK(scanned_);
  DCHECK(children->empty());

  children->resize(1);
  FourCC child_type = (*children)[0].BoxType();

  ChildMap::iterator start_itr = children_.lower_bound(child_type);
  ChildMap::iterator end_itr = children_.upper_bound(child_type);
  children->resize(std::distance(start_itr, end_itr));
  typename std::vector<T>::iterator child_itr = children->begin();
  for (ChildMap::iterator itr = start_itr; itr != end_itr; ++itr) {
    RCHECK(child_itr->Parse(&itr->second));
    ++child_itr;
  }
  children_.erase(start_itr, end_itr);

  DMX_LOG("Found %d %s boxes\n",
      children->size(),
      FourCCToString(child_type).c_str());

  return true;
}

template<typename T>
bool BoxReader::ReadAllChildren(std::vector<T>* children) {
  DCHECK(!scanned_);
  scanned_ = true;

  bool err = false;
  while (pos() < size()) {
    BoxReader child_reader(stream_, start_ + pos_, size_ - pos_);
    if (!child_reader.ReadHeader(&err)) break;
    T child;
    RCHECK(child.Parse(&child_reader));
    children->push_back(child);
    pos_ += child_reader.size();
  }

  return !err;
}

}  

#endif  
