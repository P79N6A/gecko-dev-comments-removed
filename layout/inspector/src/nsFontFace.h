



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
             uint8_t            aMatchInfo,
             nsCSSFontFaceRule* aRule);
  virtual ~nsFontFace();

  gfxFontEntry* GetFontEntry() const { return mFontEntry.get(); }

  void AddMatchType(uint8_t aMatchType) {
    mMatchType |= aMatchType;
  }

protected:
  nsRefPtr<gfxFontEntry> mFontEntry;
  nsRefPtr<nsCSSFontFaceRule> mRule;
  uint8_t mMatchType;
};

#endif 
