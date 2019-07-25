



































#ifndef __nsFontFaceList_h__
#define __nsFontFaceList_h__

#include "nsIDOMFontFaceList.h"
#include "nsIDOMFontFace.h"
#include "nsCOMPtr.h"
#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"
#include "gfxFont.h"

class gfxTextRun;

class nsFontFaceList : public nsIDOMFontFaceList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFONTFACELIST

  nsFontFaceList();
  virtual ~nsFontFaceList();

  nsresult AddFontsFromTextRun(gfxTextRun* aTextRun,
                               PRUint32 aOffset, PRUint32 aLength);

protected:
  nsInterfaceHashtable<nsPtrHashKey<gfxFontEntry>,nsIDOMFontFace> mFontFaces;
};

#endif 
