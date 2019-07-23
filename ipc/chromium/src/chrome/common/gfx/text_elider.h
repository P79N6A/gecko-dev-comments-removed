



#ifndef CHROME_COMMON_GFX_TEXT_ELIDER_H_
#define CHROME_COMMON_GFX_TEXT_ELIDER_H_

#include <unicode/coll.h>
#include <unicode/uchar.h>

#include "app/gfx/chrome_font.h"
#include "base/basictypes.h"
#include "base/string16.h"

class FilePath;
class GURL;

namespace url_parse {
struct Parsed;
}


namespace gfx {







std::wstring GetCleanStringFromUrl(const GURL& url,
                                   const std::wstring& languages,
                                   url_parse::Parsed* new_parsed,
                                   size_t* prefix_end);














std::wstring ElideUrl(const GURL& url,
                      const ChromeFont& font,
                      int available_pixel_width,
                      const std::wstring& languages);

std::wstring ElideText(const std::wstring& text,
                       const ChromeFont& font,
                       int available_pixel_width);




std::wstring ElideFilename(const FilePath& filename,
                           const ChromeFont& font,
                           int available_pixel_width);






class SortedDisplayURL {
 public:
  SortedDisplayURL(const GURL& url, const std::wstring& languages);
  SortedDisplayURL() {}

  
  
  
  int Compare(const SortedDisplayURL& other, Collator* collator) const;

  
  const string16& display_url() const { return display_url_; }

 private:
  
  
  string16 AfterHost() const;

  
  string16 sort_host_;

  
  size_t prefix_end_;

  string16 display_url_;
};

} 

#endif  
