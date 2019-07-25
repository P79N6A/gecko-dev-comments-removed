



































#ifndef __nsFontFace_h__
#define __nsFontFace_h__

#include "nsIDOMFontFace.h"

#include "gfxFont.h"

class nsFontFace : public nsIDOMFontFace
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFONTFACE

  nsFontFace(gfxFontEntry* aFontEntry);
  virtual ~nsFontFace();

  gfxFontEntry* GetFontEntry() const { return mFontEntry.get(); }

protected:
  nsRefPtr<gfxFontEntry> mFontEntry;
};

#endif 
