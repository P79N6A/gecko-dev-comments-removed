







#ifndef mozilla_ToString_h
#define mozilla_ToString_h

#include <string>
#include <sstream>

namespace mozilla {





template<typename T>
std::string
ToString(const T& t)
{
  std::ostringstream stream;
  stream << t;
  return stream.str();
}

} 

#endif 
