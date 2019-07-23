






#include "base/clipboard.h"

#include <shlobj.h>
#include <shellapi.h>

#include "base/clipboard_util.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/shared_memory.h"
#include "base/string_util.h"

namespace {


class ScopedClipboard {
 public:
  ScopedClipboard() : opened_(false) { }

  ~ScopedClipboard() {
    if (opened_)
      Release();
  }

  bool Acquire(HWND owner) {
    const int kMaxAttemptsToOpenClipboard = 5;

    if (opened_) {
      NOTREACHED();
      return false;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    

    for (int attempts = 0; attempts < kMaxAttemptsToOpenClipboard; ++attempts) {
      
      if (attempts != 0)
        ::Sleep(5);

      if (::OpenClipboard(owner)) {
        opened_ = true;
        return true;
      }
    }

    
    return false;
  }

  void Release() {
    if (opened_) {
      ::CloseClipboard();
      opened_ = false;
    } else {
      NOTREACHED();
    }
  }

 private:
  bool opened_;
};

LRESULT CALLBACK ClipboardOwnerWndProc(HWND hwnd,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam) {
  LRESULT lresult = 0;

  switch (message) {
  case WM_RENDERFORMAT:
    
    
    
    break;
  case WM_RENDERALLFORMATS:
    
    
    
    
    break;
  case WM_DRAWCLIPBOARD:
    break;
  case WM_DESTROY:
    break;
  case WM_CHANGECBCHAIN:
    break;
  default:
    lresult = DefWindowProc(hwnd, message, wparam, lparam);
    break;
  }
  return lresult;
}

template <typename charT>
HGLOBAL CreateGlobalData(const std::basic_string<charT>& str) {
  HGLOBAL data =
    ::GlobalAlloc(GMEM_MOVEABLE, ((str.size() + 1) * sizeof(charT)));
  if (data) {
    charT* raw_data = static_cast<charT*>(::GlobalLock(data));
    memcpy(raw_data, str.data(), str.size() * sizeof(charT));
    raw_data[str.size()] = '\0';
    ::GlobalUnlock(data);
  }
  return data;
};

} 

Clipboard::Clipboard() : create_window_(false) {
  if (MessageLoop::current()->type() == MessageLoop::TYPE_UI) {
    
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = ClipboardOwnerWndProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.lpszClassName = L"ClipboardOwnerWindowClass";
    ::RegisterClassEx(&wcex);
    create_window_ = true;
  }

  clipboard_owner_ = NULL;
}

Clipboard::~Clipboard() {
  if (clipboard_owner_)
    ::DestroyWindow(clipboard_owner_);
  clipboard_owner_ = NULL;
}

void Clipboard::WriteObjects(const ObjectMap& objects) {
  WriteObjects(objects, NULL);
}

void Clipboard::WriteObjects(const ObjectMap& objects,
                             base::ProcessHandle process) {
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  ::EmptyClipboard();

  for (ObjectMap::const_iterator iter = objects.begin();
       iter != objects.end(); ++iter) {
    if (iter->first == CBF_SMBITMAP)
      WriteBitmapFromSharedMemory(&(iter->second[0].front()),
                                  &(iter->second[1].front()),
                                  process);
    else
      DispatchObject(static_cast<ObjectType>(iter->first), iter->second);
  }
}

void Clipboard::WriteText(const char* text_data, size_t text_len) {
  string16 text;
  UTF8ToUTF16(text_data, text_len, &text);
  HGLOBAL glob = CreateGlobalData(text);

  WriteToClipboard(CF_UNICODETEXT, glob);
}

void Clipboard::WriteHTML(const char* markup_data,
                          size_t markup_len,
                          const char* url_data,
                          size_t url_len) {
  std::string markup(markup_data, markup_len);
  std::string url;

  if (url_len > 0)
    url.assign(url_data, url_len);

  std::string html_fragment = ClipboardUtil::HtmlToCFHtml(markup, url);
  HGLOBAL glob = CreateGlobalData(html_fragment);

  WriteToClipboard(StringToInt(GetHtmlFormatType()), glob);
}

void Clipboard::WriteBookmark(const char* title_data,
                              size_t title_len,
                              const char* url_data,
                              size_t url_len) {
  std::string bookmark(title_data, title_len);
  bookmark.append(1, L'\n');
  bookmark.append(url_data, url_len);

  string16 wide_bookmark = UTF8ToWide(bookmark);
  HGLOBAL glob = CreateGlobalData(wide_bookmark);

  WriteToClipboard(StringToInt(GetUrlWFormatType()), glob);
}

void Clipboard::WriteHyperlink(const char* title_data,
                               size_t title_len,
                               const char* url_data,
                               size_t url_len) {
  
  WriteBookmark(title_data, title_len, url_data, url_len);

  std::string title(title_data, title_len),
      url(url_data, url_len),
      link("<a href=\"");

  
  link.append(url);
  link.append("\">");
  link.append(title);
  link.append("</a>");

  
  WriteHTML(link.c_str(), link.size(), NULL, 0);
}

void Clipboard::WriteWebSmartPaste() {
  DCHECK(clipboard_owner_);
  ::SetClipboardData(StringToInt(GetWebKitSmartPasteFormatType()), NULL);
}

void Clipboard::WriteBitmap(const char* pixel_data, const char* size_data) {
  const gfx::Size* size = reinterpret_cast<const gfx::Size*>(size_data);
  HDC dc = ::GetDC(NULL);

  
  
  

  
  BITMAPINFO bm_info = {0};
  bm_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bm_info.bmiHeader.biWidth = size->width();
  bm_info.bmiHeader.biHeight = -size->height();  
  bm_info.bmiHeader.biPlanes = 1;
  bm_info.bmiHeader.biBitCount = 32;
  bm_info.bmiHeader.biCompression = BI_RGB;

  
  
  
  void *bits;
  HBITMAP source_hbitmap =
      ::CreateDIBSection(dc, &bm_info, DIB_RGB_COLORS, &bits, NULL, 0);

  if (bits && source_hbitmap) {
    
    memcpy(bits, pixel_data, 4 * size->width() * size->height());

    
    WriteBitmapFromHandle(source_hbitmap, *size);
  }

  ::DeleteObject(source_hbitmap);
  ::ReleaseDC(NULL, dc);
}

void Clipboard::WriteBitmapFromSharedMemory(const char* bitmap_data,
                                            const char* size_data,
                                            base::ProcessHandle process) {
  const gfx::Size* size = reinterpret_cast<const gfx::Size*>(size_data);

  
  
  char* ptr = const_cast<char*>(bitmap_data);
  scoped_ptr<const base::SharedMemory> bitmap(*
      reinterpret_cast<const base::SharedMemory**>(ptr));

  
  BITMAPINFO bm_info = {0};
  bm_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bm_info.bmiHeader.biWidth = size->width();
  
  bm_info.bmiHeader.biHeight = -size->height();
  bm_info.bmiHeader.biPlanes = 1;
  bm_info.bmiHeader.biBitCount = 32;
  bm_info.bmiHeader.biCompression = BI_RGB;

  HDC dc = ::GetDC(NULL);

  
  
  HBITMAP source_hbitmap =
      ::CreateDIBSection(dc, &bm_info, DIB_RGB_COLORS, NULL,
                         bitmap->handle(), 0);

  if (source_hbitmap) {
    
    WriteBitmapFromHandle(source_hbitmap, *size);
  }

  ::DeleteObject(source_hbitmap);
  ::ReleaseDC(NULL, dc);
}

void Clipboard::WriteBitmapFromHandle(HBITMAP source_hbitmap,
                                      const gfx::Size& size) {
  
  
  
  

  HDC dc = ::GetDC(NULL);
  HDC compatible_dc = ::CreateCompatibleDC(NULL);
  HDC source_dc = ::CreateCompatibleDC(NULL);

  
  HBITMAP hbitmap = ::CreateCompatibleBitmap(dc, size.width(), size.height());
  if (!hbitmap) {
    
    ::DeleteDC(compatible_dc);
    ::DeleteDC(source_dc);
    ::ReleaseDC(NULL, dc);
    return;
  }

  HBITMAP old_hbitmap = (HBITMAP)SelectObject(compatible_dc, hbitmap);
  HBITMAP old_source = (HBITMAP)SelectObject(source_dc, source_hbitmap);

  
  BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
  ::GdiAlphaBlend(compatible_dc, 0, 0, size.width(), size.height(),
                  source_dc, 0, 0, size.width(), size.height(), bf);

  
  ::SelectObject(compatible_dc, old_hbitmap);
  ::SelectObject(source_dc, old_source);
  ::DeleteObject(old_hbitmap);
  ::DeleteObject(old_source);
  ::DeleteDC(compatible_dc);
  ::DeleteDC(source_dc);
  ::ReleaseDC(NULL, dc);

  WriteToClipboard(CF_BITMAP, hbitmap);
}




void Clipboard::WriteFiles(const char* file_data, size_t file_len) {
  
  
  size_t bytes = sizeof(DROPFILES) + file_len;

  HANDLE hdata = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!hdata)
    return;

  char* data = static_cast<char*>(::GlobalLock(hdata));
  DROPFILES* drop_files = reinterpret_cast<DROPFILES*>(data);
  drop_files->pFiles = sizeof(DROPFILES);
  drop_files->fWide = TRUE;

  memcpy(data + sizeof(DROPFILES), file_data, file_len);

  ::GlobalUnlock(hdata);
  WriteToClipboard(CF_HDROP, hdata);
}

void Clipboard::WriteToClipboard(unsigned int format, HANDLE handle) {
  DCHECK(clipboard_owner_);
  if (handle && !::SetClipboardData(format, handle)) {
    DCHECK(ERROR_CLIPBOARD_NOT_OPEN != GetLastError());
    FreeData(format, handle);
  }
}

bool Clipboard::IsFormatAvailable(const Clipboard::FormatType& format) const {
  return ::IsClipboardFormatAvailable(StringToInt(format)) != FALSE;
}

void Clipboard::ReadText(string16* result) const {
  if (!result) {
    NOTREACHED();
    return;
  }

  result->clear();

  
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(CF_UNICODETEXT);
  if (!data)
    return;

  result->assign(static_cast<const char16*>(::GlobalLock(data)));
  ::GlobalUnlock(data);
}

void Clipboard::ReadAsciiText(std::string* result) const {
  if (!result) {
    NOTREACHED();
    return;
  }

  result->clear();

  
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(CF_TEXT);
  if (!data)
    return;

  result->assign(static_cast<const char*>(::GlobalLock(data)));
  ::GlobalUnlock(data);
}

void Clipboard::ReadHTML(string16* markup, std::string* src_url) const {
  if (markup)
    markup->clear();

  if (src_url)
    src_url->clear();

  
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(StringToInt(GetHtmlFormatType()));
  if (!data)
    return;

  std::string html_fragment(static_cast<const char*>(::GlobalLock(data)));
  ::GlobalUnlock(data);

  std::string markup_utf8;
  ClipboardUtil::CFHtmlToHtml(html_fragment, &markup_utf8, src_url);
  markup->assign(UTF8ToWide(markup_utf8));
}

void Clipboard::ReadBookmark(string16* title, std::string* url) const {
  if (title)
    title->clear();

  if (url)
    url->clear();

  
  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HANDLE data = ::GetClipboardData(StringToInt(GetUrlWFormatType()));
  if (!data)
    return;

  string16 bookmark(static_cast<const char16*>(::GlobalLock(data)));
  ::GlobalUnlock(data);

  ParseBookmarkClipboardFormat(bookmark, title, url);
}


void Clipboard::ReadFile(FilePath* file) const {
  if (!file) {
    NOTREACHED();
    return;
  }

  *file = FilePath();
  std::vector<FilePath> files;
  ReadFiles(&files);

  
  if (!files.empty())
    *file = files[0];
}


void Clipboard::ReadFiles(std::vector<FilePath>* files) const {
  if (!files) {
    NOTREACHED();
    return;
  }

  files->clear();

  ScopedClipboard clipboard;
  if (!clipboard.Acquire(GetClipboardWindow()))
    return;

  HDROP drop = static_cast<HDROP>(::GetClipboardData(CF_HDROP));
  if (!drop)
    return;

  
  int count = ::DragQueryFile(drop, 0xffffffff, NULL, 0);

  if (count) {
    for (int i = 0; i < count; ++i) {
      int size = ::DragQueryFile(drop, i, NULL, 0) + 1;
      std::wstring file;
      ::DragQueryFile(drop, i, WriteInto(&file, size), size);
      files->push_back(FilePath(file));
    }
  }
}


void Clipboard::ParseBookmarkClipboardFormat(const string16& bookmark,
                                             string16* title,
                                             std::string* url) {
  const string16 kDelim = ASCIIToUTF16("\r\n");

  const size_t title_end = bookmark.find_first_of(kDelim);
  if (title)
    title->assign(bookmark.substr(0, title_end));

  if (url) {
    const size_t url_start = bookmark.find_first_not_of(kDelim, title_end);
    if (url_start != string16::npos)
      *url = UTF16ToUTF8(bookmark.substr(url_start, string16::npos));
  }
}


Clipboard::FormatType Clipboard::GetUrlFormatType() {
  return IntToString(ClipboardUtil::GetUrlFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetUrlWFormatType() {
  return IntToString(ClipboardUtil::GetUrlWFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetMozUrlFormatType() {
  return IntToString(ClipboardUtil::GetMozUrlFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetPlainTextFormatType() {
  return IntToString(ClipboardUtil::GetPlainTextFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetPlainTextWFormatType() {
  return IntToString(ClipboardUtil::GetPlainTextWFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetFilenameFormatType() {
  return IntToString(ClipboardUtil::GetFilenameFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetFilenameWFormatType() {
  return IntToString(ClipboardUtil::GetFilenameWFormat()->cfFormat);
}



Clipboard::FormatType Clipboard::GetHtmlFormatType() {
  return IntToString(ClipboardUtil::GetHtmlFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetBitmapFormatType() {
  return IntToString(CF_BITMAP);
}



Clipboard::FormatType Clipboard::GetTextHtmlFormatType() {
  return IntToString(ClipboardUtil::GetTextHtmlFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetCFHDropFormatType() {
  return IntToString(ClipboardUtil::GetCFHDropFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetFileDescriptorFormatType() {
  return IntToString(ClipboardUtil::GetFileDescriptorFormat()->cfFormat);
}


Clipboard::FormatType Clipboard::GetFileContentFormatZeroType() {
  return IntToString(ClipboardUtil::GetFileContentFormatZero()->cfFormat);
}


void Clipboard::DuplicateRemoteHandles(base::ProcessHandle process,
                                       ObjectMap* objects) {
  for (ObjectMap::iterator iter = objects->begin(); iter != objects->end();
       ++iter) {
    if (iter->first == CBF_SMBITMAP) {
      
      
      char* bitmap_data = &(iter->second[0].front());
      base::SharedMemoryHandle* remote_bitmap_handle =
          reinterpret_cast<base::SharedMemoryHandle*>(bitmap_data);

      base::SharedMemory* bitmap = new base::SharedMemory(*remote_bitmap_handle,
                                                          false, process);

      
      
      iter->second[0].clear();
      for (size_t i = 0; i < sizeof(bitmap); i++)
        iter->second[0].push_back(reinterpret_cast<char*>(&bitmap)[i]);
    }
  }
}


Clipboard::FormatType Clipboard::GetWebKitSmartPasteFormatType() {
  return IntToString(ClipboardUtil::GetWebKitSmartPasteFormat()->cfFormat);
}


void Clipboard::FreeData(unsigned int format, HANDLE data) {
  if (format == CF_BITMAP)
    ::DeleteObject(static_cast<HBITMAP>(data));
  else
    ::GlobalFree(data);
}

HWND Clipboard::GetClipboardWindow() const {
  if (!clipboard_owner_ && create_window_) {
    clipboard_owner_ = ::CreateWindow(L"ClipboardOwnerWindowClass",
                                      L"ClipboardOwnerWindow",
                                      0, 0, 0, 0, 0,
                                      HWND_MESSAGE,
                                      0, 0, 0);
  }
  return clipboard_owner_;
}
