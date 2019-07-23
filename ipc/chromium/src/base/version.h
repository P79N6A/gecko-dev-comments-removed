



#ifndef BASE_VERSION_H_
#define BASE_VERSION_H_

#include <string>
#include <vector>

#include "base/basictypes.h"

class Version {
public:
  
  
  
  static Version* GetVersionFromString(const std::wstring& version_str);
  static Version* GetVersionFromString(const std::string& version_str);

  ~Version() {}

  bool Equals(const Version& other) const;

  
  int CompareTo(const Version& other) const;

  
  const std::string GetString() const;

  const std::vector<uint16>& components() const { return components_; }

private:
  Version() {}
  bool InitFromString(const std::string& version_str);

  std::vector<uint16> components_;
};

#endif  
