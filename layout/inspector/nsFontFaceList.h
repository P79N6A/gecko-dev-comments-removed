



#ifndef __nsFontFaceList_h__
#define __nsFontFaceList_h__

#include "nsIDOMFontFaceList.h"
#include "nsIDOMFontFace.h"
#include "nsCOMPtr.h"
#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"

class gfxFontEntry;
class gfxTextRun;

class nsFontFaceList : public nsIDOMFontFaceList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFONTFACELIST

  nsFontFaceList();

  nsresult AddFontsFromTextRun(gfxTextRun* aTextRun,
                               uint32_t aOffset, uint32_t aLength);

protected:
  virtual ~nsFontFaceList();

  nsInterfaceHashtable<nsPtrHashKey<gfxFontEntry>,nsIDOMFontFace> mFontFaces;
};

#endif 
