







#ifndef mozilla_ToString_h
#define mozilla_ToString_h

#include <string>
#include <sstream>

namespace mozilla {





template<typename T>
std::string
ToString(const T& aValue)
{
  std::ostringstream stream;
  stream << aValue;
  return stream.str();
}

} 

#endif 
