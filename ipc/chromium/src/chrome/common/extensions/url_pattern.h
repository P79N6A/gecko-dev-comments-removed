


#ifndef CHROME_BROWSER_EXTENSIONS_MATCH_PATTERN_H_
#define CHROME_BROWSER_EXTENSIONS_MATCH_PATTERN_H_

#include "googleurl/src/gurl.h"





























































class URLPattern {
 public:
  URLPattern() : match_subdomains_(false) {}

  
  
  bool Parse(const std::string& pattern_str);

  
  bool MatchesUrl(const GURL& url);

  std::string GetAsString() const;

  
  
  std::string scheme() const { return scheme_; }

  
  
  std::string host() const { return host_; }

  
  bool match_subdomains() const { return match_subdomains_; }

  
  
  std::string path() const { return path_; }

 private:
  
  bool MatchesHost(const GURL& test);

  
  bool MatchesPath(const GURL& test);

  
  std::string scheme_;

  
  std::string host_;

  
  
  bool match_subdomains_;

  
  
  std::string path_;

  
  
  
  std::string path_escaped_;
};

#endif  
