




#ifndef MOZILLA_GFX_RECORDINGTYPES_H_
#define MOZILLA_GFX_RECORDINGTYPES_H_

#include <ostream>

namespace mozilla {
namespace gfx {

template<class T>
struct ElementStreamFormat
{
  static void Write(std::ostream &aStream, const T &aElement)
  {
    aStream.write(reinterpret_cast<const char*>(&aElement), sizeof(T));
  }
  static void Read(std::istream &aStream, T &aElement)
  {
    aStream.read(reinterpret_cast<char *>(&aElement), sizeof(T));
  }
};

template<class T>
void WriteElement(std::ostream &aStream, const T &aElement)
{
  ElementStreamFormat<T>::Write(aStream, aElement);
}
template<class T>
void ReadElement(std::istream &aStream, T &aElement)
{
  ElementStreamFormat<T>::Read(aStream, aElement);
}

}
}

#endif 
