



#include "app/gfx/chrome_font.h"
#include "base/file_path.h"
#include "base/string_util.h"
#include "base/sys_string_conversions.h"
#include "chrome/common/gfx/text_elider.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "googleurl/src/gurl.h"
#include "net/base/escape.h"
#include "net/base/net_util.h"
#include "net/base/registry_controlled_domain.h"

const wchar_t kEllipsis[] = L"\x2026";

namespace gfx {





static void AppendFormattedHost(const GURL& url,
                                const std::wstring& languages,
                                std::wstring* output,
                                url_parse::Parsed* new_parsed);




static void AppendFormattedComponent(const std::string& spec,
                                     const url_parse::Component& in_component,
                                     std::wstring* output,
                                     url_parse::Component* out_component);











std::wstring ElideUrl(const GURL& url,
                      const ChromeFont& font,
                      int available_pixel_width,
                      const std::wstring& languages) {
  
  url_parse::Parsed parsed;
  std::wstring url_string = GetCleanStringFromUrl(url, languages, &parsed,
                                                  NULL);
  if (available_pixel_width <= 0)
    return url_string;

  
  if (!(url.SchemeIsFile() || url.IsStandard()))
    return ElideText(url_string, font, available_pixel_width);

  
  
  int pixel_width_url_string = font.GetStringWidth(url_string);
  if (available_pixel_width >= pixel_width_url_string)
    return url_string;

  
  size_t path_start_index = parsed.path.begin;
  size_t path_len = parsed.path.len;
  std::wstring url_path_query_etc = url_string.substr(path_start_index);
  std::wstring url_path = url_string.substr(path_start_index, path_len);

  
  std::wstring url_minus_query = url_string.substr(0, path_start_index +
      path_len);
  if (available_pixel_width >= font.GetStringWidth(url_minus_query))
    return ElideText(url_string, font, available_pixel_width);

  
  std::wstring url_host = UTF8ToWide(url.host());

  
  std::wstring url_domain = UTF8ToWide(
      net::RegistryControlledDomainService::GetDomainAndRegistry(url));
  if (url_domain.empty())
    url_domain = url_host;

  
  if (!url.port().empty()){
    url_host += L":" + UTF8ToWide(url.port());
    url_domain += L":" + UTF8ToWide(url.port());
  }

  
  std::wstring url_subdomain;
  size_t domain_start_index = url_host.find(url_domain);
  if (domain_start_index > 0)
    url_subdomain = url_host.substr(0, domain_start_index);
  if ((url_subdomain == L"www." || url_subdomain.empty() ||
      url.SchemeIsFile())) {
    url_subdomain.clear();
  }

  
  
  
  if (url.SchemeIsFile()) {
    
    std::vector<std::wstring> file_path_split;
    SplitString(url_path, L':', &file_path_split);
    if (file_path_split.size() > 1) { 
      url_host.clear();
      url_domain.clear();
      url_subdomain.clear();

      url_host = url_domain = file_path_split.at(0).substr(1) + L":";
      url_path_query_etc = url_path = file_path_split.at(1);
    }
  }

  
  int pixel_width_url_host = font.GetStringWidth(url_host);
  int pixel_width_url_path = font.GetStringWidth(url_path_query_etc);
  if (available_pixel_width >=
      pixel_width_url_host + pixel_width_url_path)
    return url_host + url_path_query_etc;

  
  int pixel_width_url_domain = font.GetStringWidth(url_domain);
  int pixel_width_url_subdomain = font.GetStringWidth(url_subdomain);
  if (available_pixel_width >=
      pixel_width_url_subdomain + pixel_width_url_domain +
      pixel_width_url_path)
    return url_subdomain + url_domain + url_path_query_etc;

  
  std::wstring url_query;
  const int pixel_width_dots_trailer = font.GetStringWidth(kEllipsis);
  if (parsed.query.is_nonempty()) {
    url_query = std::wstring(L"?") + url_string.substr(parsed.query.begin);
    if (available_pixel_width >= (pixel_width_url_subdomain +
        pixel_width_url_domain + pixel_width_url_path -
        font.GetStringWidth(url_query))) {
      return ElideText(url_subdomain + url_domain + url_path_query_etc, font,
                       available_pixel_width);
    }
  }

  
  std::vector<std::wstring> url_path_elements;
  SplitString(url_path, L'/', &url_path_elements);

  
  
  int url_path_number_of_elements = static_cast<int> (url_path_elements.
                                                      size());
  std::wstring url_filename;
  if ((url_path_elements.at(url_path_number_of_elements - 1)).length() > 0) {
    url_filename = *(url_path_elements.end()-1);
  } else if (url_path_number_of_elements > 1) {  
    url_filename = url_path_elements.at(url_path_number_of_elements - 2) +
        L'/';
    url_path_number_of_elements--;
  }

  const int kMaxNumberOfUrlPathElementsAllowed = 1024;
  if (url_path_number_of_elements <= 1 ||
      url_path_number_of_elements > kMaxNumberOfUrlPathElementsAllowed) {
    
    
    return ElideText(url_subdomain + url_domain + url_path_query_etc, font,
                     available_pixel_width);
  }

  
  std::wstring an_ellipsis_and_a_slash(kEllipsis);
  an_ellipsis_and_a_slash += L'/';
  int pixel_width_url_filename = font.GetStringWidth(url_filename);
  int pixel_width_dot_dot_slash = font.GetStringWidth(an_ellipsis_and_a_slash);
  int pixel_width_slash = font.GetStringWidth(L"/");
  int pixel_width_url_path_elements[kMaxNumberOfUrlPathElementsAllowed];
  for (int i = 0; i < url_path_number_of_elements; ++i) {
    pixel_width_url_path_elements[i] =
       font.GetStringWidth(url_path_elements.at(i));
  }

  
  std::wstring elided_path;
  int pixel_width_elided_path;
  for (int i = url_path_number_of_elements - 1; i >= 1; --i) {
    
    elided_path.clear();
    pixel_width_elided_path = 0;
    for (int j = 0; j < i; ++j) {
      elided_path += url_path_elements.at(j) + L'/';
      pixel_width_elided_path += pixel_width_url_path_elements[j] +
          pixel_width_slash;
    }

    
    if (i == (url_path_number_of_elements - 1)) {
      elided_path += url_filename;
      pixel_width_elided_path += pixel_width_url_filename;
    } else {
      elided_path += an_ellipsis_and_a_slash + url_filename;
      pixel_width_elided_path += pixel_width_dot_dot_slash +
          pixel_width_url_filename;
    }

    if (available_pixel_width >=
        pixel_width_url_subdomain + pixel_width_url_domain +
        pixel_width_elided_path) {
      return ElideText(url_subdomain + url_domain + elided_path + url_query,
                       font, available_pixel_width);
    }
  }

  
  
  
  
  
  std::wstring url_elided_domain = url_subdomain + url_domain;
  int pixel_width_url_elided_domain = pixel_width_url_domain;
  if (pixel_width_url_subdomain > pixel_width_dots_trailer) {
    if (!url_subdomain.empty()) {
      url_elided_domain = kEllipsis + url_domain;
      pixel_width_url_elided_domain += pixel_width_dots_trailer;
    } else {
      url_elided_domain = url_domain;
    }

    for (int i = url_path_number_of_elements - 1; i >= 1; --i) {
      
      elided_path.clear();
      pixel_width_elided_path = 0;
      for (int j = 0; j < i; ++j) {
        elided_path += url_path_elements.at(j) + L'/';
        pixel_width_elided_path += pixel_width_url_path_elements[j] +
            pixel_width_slash;
      }

      
      if (i == (url_path_number_of_elements - 1)) {
        elided_path += url_filename;
        pixel_width_elided_path += pixel_width_url_filename;
      } else {
        elided_path += an_ellipsis_and_a_slash + url_filename;
        pixel_width_elided_path += pixel_width_dot_dot_slash +
            pixel_width_url_filename;
      }

      if (available_pixel_width >=
          pixel_width_url_elided_domain + pixel_width_elided_path) {
        return ElideText(url_elided_domain + elided_path + url_query, font,
                         available_pixel_width);
      }
    }
  }

  
  std::wstring final_elided_url_string(url_elided_domain);
  if ((available_pixel_width - font.GetStringWidth(url_elided_domain)) >
      pixel_width_dot_dot_slash + pixel_width_dots_trailer +
      font.GetStringWidth(L"UV")) 
    final_elided_url_string += elided_path;
  else
    final_elided_url_string += url_path;

  return ElideText(final_elided_url_string, font, available_pixel_width);
}

std::wstring ElideFilename(const FilePath& filename,
                           const ChromeFont& font,
                           int available_pixel_width) {
  int full_width = font.GetStringWidth(filename.ToWStringHack());
  if (full_width <= available_pixel_width)
    return filename.ToWStringHack();

#if defined(OS_WIN)
  std::wstring extension = filename.Extension();
#elif defined(OS_POSIX)
  std::wstring extension = base::SysNativeMBToWide(filename.Extension());
#endif
  std::wstring rootname =
      filename.BaseName().RemoveExtension().ToWStringHack();

  if (rootname.empty() || extension.empty())
    return ElideText(filename.ToWStringHack(), font, available_pixel_width);

  int ext_width = font.GetStringWidth(extension);
  int root_width = font.GetStringWidth(rootname);

  
  if (root_width + ext_width <= available_pixel_width)
    return rootname + extension;

  int available_root_width = available_pixel_width - ext_width;
  return ElideText(rootname, font, available_root_width) + extension;
}



std::wstring ElideText(const std::wstring& text,
                       const ChromeFont& font,
                       int available_pixel_width){
  if (text.empty())
    return text;

  int current_text_pixel_width = font.GetStringWidth(text);
  if (current_text_pixel_width <= available_pixel_width)
    return text;

  if (font.GetStringWidth(kEllipsis) > available_pixel_width)
    return std::wstring();

  
  size_t lo = 0;
  size_t hi = text.length() - 1;
  size_t guess = hi / 2;
  while (lo < hi) {
    
    
    std::wstring guess_str = text.substr(0, guess) + kEllipsis;
    int guess_length = font.GetStringWidth(guess_str);
    if (guess_length > available_pixel_width) {
      if (hi == guess)
        break;
      hi = guess;
    } else {
      if (lo == guess)
        break;
      lo = guess;
    }
    guess = (lo + hi) / 2;
  }

  return text.substr(0, lo) + kEllipsis;
}

void AppendFormattedHost(const GURL& url,
                         const std::wstring& languages,
                         std::wstring* output,
                         url_parse::Parsed* new_parsed) {
  const url_parse::Component& host =
      url.parsed_for_possibly_invalid_spec().host;

  if (host.is_nonempty()) {
    
    if (new_parsed)
      new_parsed->host.begin = static_cast<int>(output->length());

    const std::string& spec = url.possibly_invalid_spec();
    DCHECK(host.begin >= 0 &&
           ((spec.length() == 0 && host.begin == 0) ||
            host.begin < static_cast<int>(spec.length())));
    net::IDNToUnicode(&spec[host.begin], host.len, languages, output);

    if (new_parsed) {
      new_parsed->host.len =
          static_cast<int>(output->length()) - new_parsed->host.begin;
    }
  } else if (new_parsed) {
    new_parsed->host.reset();
  }
}

void AppendFormattedComponent(const std::string& spec,
                              const url_parse::Component& in_component,
                              std::wstring* output,
                              url_parse::Component* out_component) {
  if (in_component.is_nonempty()) {
    out_component->begin = static_cast<int>(output->length());

    output->append(UnescapeAndDecodeUTF8URLComponent(
        spec.substr(in_component.begin, in_component.len),
        UnescapeRule::NORMAL));

    out_component->len =
        static_cast<int>(output->length()) - out_component->begin;
  } else {
    out_component->reset();
  }
}

std::wstring GetCleanStringFromUrl(const GURL& url,
                                   const std::wstring& languages,
                                   url_parse::Parsed* new_parsed,
                                   size_t* prefix_end) {
  url_parse::Parsed parsed_temp;
  if (!new_parsed)
    new_parsed = &parsed_temp;

  std::wstring url_string;

  
  if (url.is_empty()) {
    if (prefix_end)
      *prefix_end = 0;
    return url_string;
  }

  
  
  const std::string& spec = url.possibly_invalid_spec();
  const url_parse::Parsed& parsed = url.parsed_for_possibly_invalid_spec();

  
  
  
  
  
  
  int pre_end = parsed.CountCharactersBefore(
      url_parse::Parsed::USERNAME, true);
  for (int i = 0; i < pre_end; ++i)
    url_string.push_back(spec[i]);
  if (prefix_end)
    *prefix_end = static_cast<size_t>(pre_end);
  new_parsed->scheme = parsed.scheme;
  new_parsed->username.reset();
  new_parsed->password.reset();

  AppendFormattedHost(url, languages, &url_string, new_parsed);

  
  if (parsed.port.is_nonempty()) {
    url_string.push_back(':');
    for (int i = parsed.port.begin; i < parsed.port.end(); ++i)
      url_string.push_back(spec[i]);
  }

  
  AppendFormattedComponent(spec, parsed.path, &url_string, &new_parsed->path);
  if (parsed.query.is_valid())
    url_string.push_back('?');
  AppendFormattedComponent(spec, parsed.query, &url_string, &new_parsed->query);

  
  if (parsed.ref.is_valid()) {
    url_string.push_back('#');
    if (parsed.ref.len > 0)
      url_string.append(UTF8ToWide(std::string(&spec[parsed.ref.begin],
                                               parsed.ref.len)));
  }

  return url_string;
}

SortedDisplayURL::SortedDisplayURL(const GURL& url,
                                   const std::wstring& languages) {
  std::wstring host;
  AppendFormattedHost(url, languages, &host, NULL);
  sort_host_ = WideToUTF16Hack(host);
  string16 host_minus_www = WideToUTF16Hack(net::StripWWW(host));
  url_parse::Parsed parsed;
  display_url_ = WideToUTF16Hack(GetCleanStringFromUrl(url, languages,
      &parsed, &prefix_end_));
  if (sort_host_.length() > host_minus_www.length()) {
    prefix_end_ += sort_host_.length() - host_minus_www.length();
    sort_host_.swap(host_minus_www);
  }
}

int SortedDisplayURL::Compare(const SortedDisplayURL& other,
                              Collator* collator) const {
  
  UErrorCode compare_status = U_ZERO_ERROR;
  UCollationResult host_compare_result = collator->compare(
      static_cast<const UChar*>(sort_host_.c_str()),
      static_cast<int>(sort_host_.length()),
      static_cast<const UChar*>(other.sort_host_.c_str()),
      static_cast<int>(other.sort_host_.length()),
      compare_status);
  DCHECK(U_SUCCESS(compare_status));
  if (host_compare_result != 0)
    return host_compare_result;

  
  string16 path = this->AfterHost();
  string16 o_path = other.AfterHost();
  compare_status = U_ZERO_ERROR;
  UCollationResult path_compare_result = collator->compare(
      static_cast<const UChar*>(path.c_str()),
      static_cast<int>(path.length()),
      static_cast<const UChar*>(o_path.c_str()),
      static_cast<int>(o_path.length()),
      compare_status);
  DCHECK(U_SUCCESS(compare_status));
  if (path_compare_result != 0)
    return path_compare_result;

  
  
  compare_status = U_ZERO_ERROR;
  UCollationResult display_url_compare_result = collator->compare(
      static_cast<const UChar*>(display_url_.c_str()),
      static_cast<int>(display_url_.length()),
      static_cast<const UChar*>(other.display_url_.c_str()),
      static_cast<int>(other.display_url_.length()),
      compare_status);
  DCHECK(U_SUCCESS(compare_status));
  return display_url_compare_result;
}

string16 SortedDisplayURL::AfterHost() const {
  size_t slash_index = display_url_.find(sort_host_, prefix_end_);
  if (slash_index == string16::npos) {
    NOTREACHED();
    return string16();
  }
  return display_url_.substr(slash_index + sort_host_.length());
}

} 
