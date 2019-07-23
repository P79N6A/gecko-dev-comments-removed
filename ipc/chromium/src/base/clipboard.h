



#ifndef BASE_CLIPBOARD_H_
#define BASE_CLIPBOARD_H_

#include <map>
#include <string>
#include <vector>

#include "base/file_path.h"
#include "base/process.h"
#include "base/string16.h"
#include "base/gfx/size.h"

class Clipboard {
 public:
  typedef std::string FormatType;
#if defined(OS_LINUX)
  typedef struct _GtkClipboard GtkClipboard;
  typedef std::map<FormatType, std::pair<char*, size_t> > TargetMap;
#endif

  
  
  
  
  
  
  enum ObjectType {
    CBF_TEXT,
    CBF_HTML,
    CBF_BOOKMARK,
    CBF_LINK,
    CBF_FILES,
    CBF_WEBKIT,
    CBF_BITMAP,
    CBF_SMBITMAP 
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef std::vector<char> ObjectMapParam;
  typedef std::vector<ObjectMapParam> ObjectMapParams;
  typedef std::map<int , ObjectMapParams> ObjectMap;

  Clipboard();
  ~Clipboard();

  
  
  
  
  void WriteObjects(const ObjectMap& objects);

  
  
  
  
  void WriteObjects(const ObjectMap& objects, base::ProcessHandle process);

  
  bool IsFormatAvailable(const FormatType& format) const;

  
  void ReadText(string16* result) const;

  
  void ReadAsciiText(std::string* result) const;

  
  void ReadHTML(string16* markup, std::string* src_url) const;

  
  void ReadBookmark(string16* title, std::string* url) const;

  
  
  void ReadFile(FilePath* file) const;
  void ReadFiles(std::vector<FilePath>* files) const;

  
  static FormatType GetUrlFormatType();
  static FormatType GetUrlWFormatType();
  static FormatType GetMozUrlFormatType();
  static FormatType GetPlainTextFormatType();
  static FormatType GetPlainTextWFormatType();
  static FormatType GetFilenameFormatType();
  static FormatType GetFilenameWFormatType();
  static FormatType GetWebKitSmartPasteFormatType();
  
  static FormatType GetHtmlFormatType();
#if defined(OS_WIN)
  static FormatType GetBitmapFormatType();
  
  static FormatType GetTextHtmlFormatType();
  static FormatType GetCFHDropFormatType();
  static FormatType GetFileDescriptorFormatType();
  static FormatType GetFileContentFormatZeroType();

  
  
  static void DuplicateRemoteHandles(base::ProcessHandle process,
                                     ObjectMap* objects);
#endif

 private:
  void WriteText(const char* text_data, size_t text_len);

  void WriteHTML(const char* markup_data,
                 size_t markup_len,
                 const char* url_data,
                 size_t url_len);

  void WriteBookmark(const char* title_data,
                     size_t title_len,
                     const char* url_data,
                     size_t url_len);

  void WriteHyperlink(const char* title_data,
                      size_t title_len,
                      const char* url_data,
                      size_t url_len);

  void WriteWebSmartPaste();

  void WriteFiles(const char* file_data, size_t file_len);

  void DispatchObject(ObjectType type, const ObjectMapParams& params);

  void WriteBitmap(const char* pixel_data, const char* size_data);
#if defined(OS_WIN)
  void WriteBitmapFromSharedMemory(const char* bitmap_data,
                                   const char* size_data,
                                   base::ProcessHandle handle);

  void WriteBitmapFromHandle(HBITMAP source_hbitmap,
                             const gfx::Size& size);

  
  void WriteToClipboard(unsigned int format, HANDLE handle);

  static void ParseBookmarkClipboardFormat(const string16& bookmark,
                                           string16* title,
                                           std::string* url);

  
  static void FreeData(unsigned int format, HANDLE data);

  
  
  HWND GetClipboardWindow() const;

  
  mutable HWND clipboard_owner_;

  
  bool create_window_;
#elif defined(OS_LINUX)
  
  
  
  
  
  

  
  void SetGtkClipboard();
  
  void FreeTargetMap();
  
  void InsertMapping(const char* key, char* data, size_t data_len);

  TargetMap* clipboard_data_;
  GtkClipboard* clipboard_;
#endif

  DISALLOW_EVIL_CONSTRUCTORS(Clipboard);
};

#endif  
