








#ifndef BASE_SCOPED_CLIPBOARD_WRITER_H_
#define BASE_SCOPED_CLIPBOARD_WRITER_H_

#include <string>
#include <vector>

#include "base/clipboard.h"
#include "base/file_path.h"
#include "base/string16.h"





class ScopedClipboardWriter {
 public:
  
  ScopedClipboardWriter(Clipboard* clipboard);

  ~ScopedClipboardWriter();

  
  void WriteText(const string16& text);

  
  
  void WriteHTML(const string16& markup, const std::string& source_url);

  
  void WriteBookmark(const string16& bookmark_title,
                     const std::string& url);

  
  
  
  void WriteHyperlink(const string16& link_text, const std::string& url);

  
  void WriteFile(const FilePath& file);
  void WriteFiles(const std::vector<FilePath>& files);

  
  void WriteWebSmartPaste();

  
  
  void WriteBitmapFromPixels(const void* pixels, const gfx::Size& size);

 protected:
  
  
  Clipboard::ObjectMap objects_;
  Clipboard* clipboard_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedClipboardWriter);
};

#endif  
