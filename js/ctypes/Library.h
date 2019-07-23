






































#ifndef LIBRARY_H
#define LIBRARY_H

#include "nsIForeignLibrary.h"

#define FOREIGNLIBRARY_CONTRACTID \
  "@mozilla.org/jsctypes;1"

#define FOREIGNLIBRARY_CID \
{ 0xc797702, 0x1c60, 0x4051, { 0x9d, 0xd7, 0x4d, 0x74, 0x5, 0x60, 0x56, 0x42 } }

struct PRLibrary;

namespace mozilla {
namespace ctypes {

class Library : public nsIForeignLibrary
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFOREIGNLIBRARY

  Library();

  bool IsOpen() { return mLibrary != nsnull; }

private:
  ~Library();

  PRLibrary* mLibrary;
};

}
}

#endif
