






































#ifndef NSNATIVETYPES_H
#define NSNATIVETYPES_H

#include "nsINativeTypes.h"

#define NATIVETYPES_CONTRACTID \
  "@mozilla.org/jsctypes;1"

#define NATIVETYPES_CID \
{ 0xc797702, 0x1c60, 0x4051, { 0x9d, 0xd7, 0x4d, 0x74, 0x5, 0x60, 0x56, 0x42 } }

struct PRLibrary;

class nsNativeTypes : public nsINativeTypes
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINATIVETYPES

  nsNativeTypes();

  PRBool IsOpen() { return mLibrary != nsnull; }

private:
  ~nsNativeTypes();

  PRLibrary* mLibrary;
};

#endif
