



#ifndef CHROME_COMMON_GFX_EMF_H__
#define CHROME_COMMON_GFX_EMF_H__

#include <windows.h>
#include <vector>

#include "base/basictypes.h"

namespace gfx {

class Rect;


class Emf {
 public:
  class Record;
  class Enumerator;
  struct EnumerationContext;

  Emf();
  ~Emf();

  
  
  
  
  
  bool CreateDc(HDC sibling, const RECT* rect);

  
  bool CreateFromData(const void* buffer, size_t size);

  
  
  

  
  
  bool CloseDc();

  
  void CloseEmf();

  
  
  
  
  
  
  
  
  bool Playback(HDC hdc, const RECT* rect) const;

  
  
  
  
  bool SafePlayback(HDC hdc) const;

  
  
  gfx::Rect GetBounds() const;

  
  unsigned GetDataSize() const;

  
  bool GetData(void* buffer, size_t size) const;

  
  bool GetData(std::vector<uint8>* buffer) const;

  HENHMETAFILE emf() const {
    return emf_;
  }

  HDC hdc() const {
    return hdc_;
  }

  
  
  
  bool SaveTo(const std::wstring& filename) const;

 private:
  
  static int CALLBACK SafePlaybackProc(HDC hdc,
                                       HANDLETABLE* handle_table,
                                       const ENHMETARECORD* record,
                                       int objects_count,
                                       LPARAM param);

  
  HENHMETAFILE emf_;

  
  HDC hdc_;

  DISALLOW_EVIL_CONSTRUCTORS(Emf);
};

struct Emf::EnumerationContext {
  HANDLETABLE* handle_table;
  int objects_count;
  HDC hdc;
};



class Emf::Record {
 public:
  Record();

  
  bool Play() const;

  
  
  bool SafePlayback(const XFORM* base_matrix) const;

  
  const ENHMETARECORD* record() const { return record_; }

 protected:
  Record(const EnumerationContext* context,
         const ENHMETARECORD* record);

 private:
  friend class Emf;
  friend class Enumerator;
  const ENHMETARECORD* record_;
  const EnumerationContext* context_;
};




class Emf::Enumerator {
 public:
  
  typedef std::vector<Record>::const_iterator const_iterator;

  
  
  
  Enumerator(const Emf& emf, HDC hdc, const RECT* rect);

  
  const_iterator begin() const;

  
  const_iterator end() const;

 private:
  
  static int CALLBACK EnhMetaFileProc(HDC hdc,
                                      HANDLETABLE* handle_table,
                                      const ENHMETARECORD* record,
                                      int objects_count,
                                      LPARAM param);

  
  
  
  std::vector<Record> items_;

  EnumerationContext context_;

  DISALLOW_EVIL_CONSTRUCTORS(Enumerator);
};

}  

#endif  
