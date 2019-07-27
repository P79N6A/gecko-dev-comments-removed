







#ifndef nsFontFaceUtils_h_
#define nsFontFaceUtils_h_

class gfxUserFontEntry;
class nsIFrame;

class nsFontFaceUtils
{
public:
  
  static void MarkDirtyForFontChange(nsIFrame* aSubtreeRoot,
                                     const gfxUserFontEntry* aFont);
};

#endif 
