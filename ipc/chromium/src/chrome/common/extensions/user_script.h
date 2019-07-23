



#ifndef CHROME_COMMON_EXTENSIONS_USER_SCRIPT_H_
#define CHROME_COMMON_EXTENSIONS_USER_SCRIPT_H_

#include <vector>
#include <string>

#include "base/file_path.h"
#include "base/string_piece.h"
#include "chrome/common/extensions/url_pattern.h"
#include "googleurl/src/gurl.h"

class Pickle;



class UserScript {
 public:
  
  enum RunLocation {
    DOCUMENT_START,  
                     
    DOCUMENT_END,  
                   

    RUN_LOCATION_LAST  
  };

  
  class File {
   public:
    File(const FilePath& path, const GURL& url):
         path_(path),
         url_(url) {
    }
    File() {}

    const FilePath& path() const { return path_; }
    void set_path(const FilePath& path) { path_ = path; }

    const GURL& url() const { return url_; }
    void set_url(const GURL& url) { url_ = url; }

    
    
    const StringPiece GetContent() const {
      if (external_content_.data())
        return external_content_;
      else
        return content_;
    }
    void set_external_content(const StringPiece& content) {
      external_content_ = content;
    }
    const void set_content(const StringPiece& content) {
      content_.assign(content.begin(), content.end());
    }

    
    
    void Pickle(::Pickle* pickle) const;
    void Unpickle(const ::Pickle& pickle, void** iter);

   private:
    
    FilePath path_;

    
    GURL url_;

    
    
    StringPiece external_content_;

    
    std::string content_;
  };

  typedef std::vector<File> FileList;

  
  
  UserScript() : run_location_(DOCUMENT_END) {}

  
  RunLocation run_location() const { return run_location_; }
  void set_run_location(RunLocation location) { run_location_ = location; }

  
  
  const std::vector<std::string>& globs() const { return globs_; }
  void add_glob(const std::string& glob) { globs_.push_back(glob); }
  void clear_globs() { globs_.clear(); }

  
  
  const std::vector<URLPattern>& url_patterns() const { return url_patterns_; }
  void add_url_pattern(const URLPattern& pattern) {
    url_patterns_.push_back(pattern);
  }
  void clear_url_patterns() { url_patterns_.clear(); }

  
  FileList& js_scripts() { return js_scripts_; }
  const FileList& js_scripts() const { return js_scripts_; }

  
  FileList& css_scripts() { return css_scripts_; }
  const FileList& css_scripts() const { return css_scripts_; }

  const std::string& extension_id() const { return extension_id_; }
  void set_extension_id(const std::string& id) { extension_id_ = id; }

  bool is_standalone() { return extension_id_.empty(); }

  
  
  bool MatchesUrl(const GURL& url);

  
  
  void Pickle(::Pickle* pickle) const;

  
  
  
  void Unpickle(const ::Pickle& pickle, void** iter);

 private:
  
  RunLocation run_location_;

  
  
  std::vector<std::string> globs_;

  
  
  std::vector<URLPattern> url_patterns_;

  
  FileList js_scripts_;

  
  FileList css_scripts_;

  
  
  std::string extension_id_;
};

typedef std::vector<UserScript> UserScriptList;

#endif
