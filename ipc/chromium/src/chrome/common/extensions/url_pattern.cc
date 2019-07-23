



#include "chrome/common/extensions/url_pattern.h"

#include "base/string_piece.h"
#include "base/string_util.h"
#include "chrome/common/url_constants.h"



static const char* kValidSchemes[] = {
  chrome::kHttpScheme,
  chrome::kHttpsScheme,
  chrome::kFileScheme,
  chrome::kFtpScheme,
  chrome::kChromeUIScheme,
};

static const char kPathSeparator[] = "/";

static bool IsValidScheme(const std::string& scheme) {
  for (size_t i = 0; i < arraysize(kValidSchemes); ++i) {
    if (scheme == kValidSchemes[i])
      return true;
  }

  return false;
}

bool URLPattern::Parse(const std::string& pattern) {
  size_t scheme_end_pos = pattern.find(chrome::kStandardSchemeSeparator);
  if (scheme_end_pos == std::string::npos)
    return false;

  scheme_ = pattern.substr(0, scheme_end_pos);
  if (!IsValidScheme(scheme_))
    return false;

  size_t host_start_pos = scheme_end_pos +
      strlen(chrome::kStandardSchemeSeparator);
  if (host_start_pos >= pattern.length())
    return false;

  
  size_t path_start_pos = 0;

  
  
  if (scheme_ == "file") {
    path_start_pos = host_start_pos;
  } else {
    size_t host_end_pos = pattern.find(kPathSeparator, host_start_pos);
    if (host_end_pos == std::string::npos)
      return false;

    host_ = pattern.substr(host_start_pos, host_end_pos - host_start_pos);

    
    std::vector<std::string> host_components;
    SplitString(host_, '.', &host_components);
    if (host_components[0] == "*") {
      match_subdomains_ = true;
      host_components.erase(host_components.begin(),
                            host_components.begin() + 1);
    }
    host_ = JoinString(host_components, '.');

    
    
    
    if (host_.find('*') != std::string::npos)
      return false;

    path_start_pos = host_end_pos;
  }

  path_ = pattern.substr(path_start_pos);
  return true;
}

bool URLPattern::MatchesUrl(const GURL &test) {
  if (test.scheme() != scheme_)
    return false;

  if (!MatchesHost(test))
    return false;

  if (!MatchesPath(test))
    return false;

  return true;
}

bool URLPattern::MatchesHost(const GURL& test) {
  if (test.host() == host_)
    return true;

  if (!match_subdomains_ || test.HostIsIPAddress())
    return false;

  
  
  if (host_.empty())
    return true;

  
  if (test.host().length() <= (host_.length() + 1))
    return false;

  if (test.host().compare(test.host().length() - host_.length(),
                          host_.length(), host_) != 0)
    return false;

  return test.host()[test.host().length() - host_.length() - 1] == '.';
}

bool URLPattern::MatchesPath(const GURL& test) {
  if (path_escaped_.empty()) {
    path_escaped_ = path_;
    ReplaceSubstringsAfterOffset(&path_escaped_, 0, "\\", "\\\\");
    ReplaceSubstringsAfterOffset(&path_escaped_, 0, "?", "\\?");
  }

  if (!MatchPattern(test.PathForRequest(), path_escaped_))
    return false;

  return true;
}

std::string URLPattern::GetAsString() const {
  std::string spec = scheme_ + chrome::kStandardSchemeSeparator;

  if (match_subdomains_) {
    spec += "*";
    if (!host_.empty())
      spec += ".";
  }

  if (!host_.empty())
    spec += host_;

  if (!path_.empty())
    spec += path_;

  return spec;
}
