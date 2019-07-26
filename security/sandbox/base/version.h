



#ifndef BASE_VERSION_H_
#define BASE_VERSION_H_

#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/basictypes.h"

namespace base {



class BASE_EXPORT Version {
 public:
  
  
  Version();

  ~Version();

  
  
  
  explicit Version(const std::string& version_str);

  
  bool IsValid() const;

  
  
  
  
  static bool IsValidWildcardString(const std::string& wildcard_string);

  
  
  
  
  bool IsOlderThan(const std::string& version_str) const;

  bool Equals(const Version& other) const;

  
  int CompareTo(const Version& other) const;

  
  
  
  
  int CompareToWildcardString(const std::string& wildcard_string) const;

  
  const std::string GetString() const;

  const std::vector<uint16>& components() const { return components_; }

 private:
  std::vector<uint16> components_;
};

}  



using base::Version;

#endif  
