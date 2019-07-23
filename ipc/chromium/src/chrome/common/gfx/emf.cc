



#include "chrome/common/gfx/emf.h"

#include "base/gfx/rect.h"
#include "base/logging.h"

namespace gfx {

Emf::Emf() : emf_(NULL), hdc_(NULL) {
}

Emf::~Emf() {
  CloseEmf();
  DCHECK(!emf_ && !hdc_);
}

bool Emf::CreateDc(HDC sibling, const RECT* rect) {
  DCHECK(!emf_ && !hdc_);
  hdc_ = CreateEnhMetaFile(sibling, NULL, rect, NULL);
  DCHECK(hdc_);
  return hdc_ != NULL;
}

bool Emf::CreateFromData(const void* buffer, size_t size) {
  DCHECK(!emf_ && !hdc_);
  emf_ = SetEnhMetaFileBits(static_cast<unsigned>(size),
                            reinterpret_cast<const BYTE*>(buffer));
  DCHECK(emf_);
  return emf_ != NULL;
}

bool Emf::CloseDc() {
  DCHECK(!emf_ && hdc_);
  emf_ = CloseEnhMetaFile(hdc_);
  DCHECK(emf_);
  hdc_ = NULL;
  return emf_ != NULL;
}

void Emf::CloseEmf() {
  DCHECK(!hdc_);
  if (emf_) {
    DeleteEnhMetaFile(emf_);
    emf_ = NULL;
  }
}

bool Emf::Playback(HDC hdc, const RECT* rect) const {
  DCHECK(emf_ && !hdc_);
  RECT bounds;
  if (!rect) {
    
    bounds = GetBounds().ToRECT();
    rect = &bounds;
  }
  return PlayEnhMetaFile(hdc, emf_, rect) != 0;
}

bool Emf::SafePlayback(HDC context) const {
  DCHECK(emf_ && !hdc_);
  XFORM base_matrix;
  if (!GetWorldTransform(context, &base_matrix)) {
    NOTREACHED();
    return false;
  }

  return EnumEnhMetaFile(context,
                         emf_,
                         &Emf::SafePlaybackProc,
                         reinterpret_cast<void*>(&base_matrix),
                         &GetBounds().ToRECT()) != 0;
}

gfx::Rect Emf::GetBounds() const {
  DCHECK(emf_ && !hdc_);
  ENHMETAHEADER header;
  if (GetEnhMetaFileHeader(emf_, sizeof(header), &header) != sizeof(header)) {
    NOTREACHED();
    return gfx::Rect();
  }
  if (header.rclBounds.left == 0 &&
      header.rclBounds.top == 0 &&
      header.rclBounds.right == -1 &&
      header.rclBounds.bottom == -1) {
    
    
    
    return gfx::Rect();
  }
  return gfx::Rect(header.rclBounds.left,
                   header.rclBounds.top,
                   header.rclBounds.right - header.rclBounds.left,
                   header.rclBounds.bottom - header.rclBounds.top);
}

unsigned Emf::GetDataSize() const {
  DCHECK(emf_ && !hdc_);
  return GetEnhMetaFileBits(emf_, 0, NULL);
}

bool Emf::GetData(void* buffer, size_t size) const {
  DCHECK(emf_ && !hdc_);
  DCHECK(buffer && size);
  unsigned size2 = GetEnhMetaFileBits(emf_, static_cast<unsigned>(size),
                                      reinterpret_cast<BYTE*>(buffer));
  DCHECK(size2 == size);
  return size2 == size && size2 != 0;
}

bool Emf::GetData(std::vector<uint8>* buffer) const {
  unsigned size = GetDataSize();
  if (!size)
    return false;

  buffer->resize(size);
  if (!GetData(&buffer->front(), size))
    return false;
  return true;
}

bool Emf::SaveTo(const std::wstring& filename) const {
  HANDLE file = CreateFile(filename.c_str(), GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           CREATE_ALWAYS, 0, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return false;

  bool success = false;
  std::vector<uint8> buffer;
  if (GetData(&buffer)) {
    DWORD written = 0;
    if (WriteFile(file, &*buffer.begin(), static_cast<DWORD>(buffer.size()),
                  &written, NULL) &&
        written == buffer.size()) {
      success = true;
    }
  }
  CloseHandle(file);
  return success;
}

int CALLBACK Emf::SafePlaybackProc(HDC hdc,
                                   HANDLETABLE* handle_table,
                                   const ENHMETARECORD* record,
                                   int objects_count,
                                   LPARAM param) {
  const XFORM* base_matrix = reinterpret_cast<const XFORM*>(param);
  EnumerationContext context;
  context.handle_table = handle_table;
  context.objects_count = objects_count;
  context.hdc = hdc;
  Record record_instance(&context, record);
  bool success = record_instance.SafePlayback(base_matrix);
  DCHECK(success);
  return 1;
}

Emf::Record::Record() {
}

Emf::Record::Record(const EnumerationContext* context,
                    const ENHMETARECORD* record)
    : record_(record),
      context_(context) {
  DCHECK(record_);
}

bool Emf::Record::Play() const {
  return 0 != PlayEnhMetaFileRecord(context_->hdc,
                                    context_->handle_table,
                                    record_,
                                    context_->objects_count);
}

bool Emf::Record::SafePlayback(const XFORM* base_matrix) const {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool res;
  switch (record()->iType) {
    case EMR_SETWORLDTRANSFORM: {
      DCHECK_EQ(record()->nSize, sizeof(DWORD) * 2 + sizeof(XFORM));
      const XFORM* xform = reinterpret_cast<const XFORM*>(record()->dParm);
      HDC hdc = context_->hdc;
      if (base_matrix) {
        res = 0 != SetWorldTransform(hdc, base_matrix) &&
                   ModifyWorldTransform(hdc, xform, MWT_LEFTMULTIPLY);
      } else {
        res = 0 != SetWorldTransform(hdc, xform);
      }
      break;
    }
    case EMR_MODIFYWORLDTRANSFORM: {
      DCHECK_EQ(record()->nSize,
                sizeof(DWORD) * 2 + sizeof(XFORM) + sizeof(DWORD));
      const XFORM* xform = reinterpret_cast<const XFORM*>(record()->dParm);
      const DWORD* option = reinterpret_cast<const DWORD*>(xform + 1);
      HDC hdc = context_->hdc;
      switch (*option) {
        case MWT_IDENTITY:
          if (base_matrix) {
            res = 0 != SetWorldTransform(hdc, base_matrix);
          } else {
            res = 0 != ModifyWorldTransform(hdc, xform, MWT_IDENTITY);
          }
          break;
        case MWT_LEFTMULTIPLY:
        case MWT_RIGHTMULTIPLY:
          res = 0 != ModifyWorldTransform(hdc, xform, *option);
          break;
        case 4:  
          if (base_matrix) {
            res = 0 != SetWorldTransform(hdc, base_matrix) &&
                       ModifyWorldTransform(hdc, xform, MWT_LEFTMULTIPLY);
          } else {
            res = 0 != SetWorldTransform(hdc, xform);
          }
          break;
        default:
          res = false;
          break;
      }
      break;
    }
    case EMR_SETLAYOUT:
      
      res = true;
      break;
    default: {
      res = Play();
      break;
    }
  }
  return res;
}

Emf::Enumerator::Enumerator(const Emf& emf, HDC context, const RECT* rect) {
  context_.handle_table = NULL;
  context_.objects_count = 0;
  context_.hdc = NULL;
  items_.clear();
  if (!EnumEnhMetaFile(context,
                       emf.emf(),
                       &Emf::Enumerator::EnhMetaFileProc,
                       reinterpret_cast<void*>(this),
                       rect)) {
    NOTREACHED();
    items_.clear();
  }
  DCHECK_EQ(context_.hdc, context);
}

Emf::Enumerator::const_iterator Emf::Enumerator::begin() const {
  return items_.begin();
}

Emf::Enumerator::const_iterator Emf::Enumerator::end() const {
  return items_.end();
}

int CALLBACK Emf::Enumerator::EnhMetaFileProc(HDC hdc,
                                              HANDLETABLE* handle_table,
                                              const ENHMETARECORD* record,
                                              int objects_count,
                                              LPARAM param) {
  Enumerator& emf = *reinterpret_cast<Enumerator*>(param);
  if (!emf.context_.handle_table) {
    DCHECK(!emf.context_.handle_table);
    DCHECK(!emf.context_.objects_count);
    emf.context_.handle_table = handle_table;
    emf.context_.objects_count = objects_count;
    emf.context_.hdc = hdc;
  } else {
    DCHECK_EQ(emf.context_.handle_table, handle_table);
    DCHECK_EQ(emf.context_.objects_count, objects_count);
    DCHECK_EQ(emf.context_.hdc, hdc);
  }
  emf.items_.push_back(Record(&emf.context_, record));
  return 1;
}

}  
