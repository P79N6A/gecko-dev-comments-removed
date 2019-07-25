



































#ifndef __nsFontFace_h__
#define __nsFontFace_h__

#include "nsIDOMFontFace.h"

#include "gfxFont.h"

class nsCSSFontFaceRule;

class nsFontFace : public nsIDOMFontFace
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMFONTFACE

  nsFontFace(gfxFontEntry*      aFontEntry,
             PRUint8            aMatchInfo,
             nsCSSFontFaceRule* aRule);
  virtual ~nsFontFace();

  gfxFontEntry* GetFontEntry() const { return mFontEntry.get(); }

  void AddMatchType(PRUint8 aMatchType) {
    mMatchType |= aMatchType;
  }

protected:
  nsRefPtr<gfxFontEntry> mFontEntry;
  nsRefPtr<nsCSSFontFaceRule> mRule;
  PRUint8 mMatchType;
};

#endif 
