





#ifndef _SDPERRORHOLDER_H_
#define _SDPERRORHOLDER_H_

#include <vector>
#include <string>

namespace mozilla
{

class SdpErrorHolder
{
public:
  SdpErrorHolder() {}
  virtual ~SdpErrorHolder() {}

  void
  AddParseError(size_t line, const std::string& message)
  {
    mErrors.push_back(std::make_pair(line, message));
  }

  void
  ClearParseErrors()
  {
    mErrors.clear();
  }

  



  const std::vector<std::pair<size_t, std::string> >&
  GetParseErrors() const
  {
    return mErrors;
  }

private:
  std::vector<std::pair<size_t, std::string> > mErrors;
};

} 

#endif
