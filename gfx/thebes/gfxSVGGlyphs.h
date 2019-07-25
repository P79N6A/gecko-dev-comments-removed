



































#ifndef GFX_SVG_GLYPHS_WRAPPER_H
#define GFX_SVG_GLYPHS_WRAPPER_H

#include "gfxFontUtils.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsAutoPtr.h"
#include "nsIContentViewer.h"
#include "nsIPresShell.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsIDocument.h"
#include "gfxPattern.h"
#include "gfxFont.h"






class gfxSVGGlyphs {
private:
    typedef mozilla::dom::Element Element;
    typedef gfxFont::DrawMode DrawMode;

public:
    static gfxSVGGlyphs* ParseFromBuffer(uint8_t *aBuffer, uint32_t aBufLen);

    bool HasSVGGlyph(uint32_t aGlyphId);

    bool RenderGlyph(gfxContext *aContext, uint32_t aGlyphId, DrawMode aDrawMode);

    bool Init(const gfxFontEntry *aFont,
              const FallibleTArray<uint8_t> &aCmapTable);

    ~gfxSVGGlyphs() {
        mViewer->Destroy();
    }

private:
    gfxSVGGlyphs() {
    }

    static nsresult ParseDocument(uint8_t *aBuffer, uint32_t aBufLen,
                                  nsIDocument **aResult);

    void FindGlyphElements(Element *aElement, const gfxFontEntry *aFontEntry,
                           const FallibleTArray<uint8_t> &aCmapTable);

    void InsertGlyphId(Element *aGlyphElement);
    void InsertGlyphChar(Element *aGlyphElement, const gfxFontEntry *aFontEntry,
                         const FallibleTArray<uint8_t> &aCmapTable);
    
    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsIContentViewer> mViewer;
    nsCOMPtr<nsIPresShell> mPresShell;

    nsBaseHashtable<nsUint32HashKey, Element*, Element*> mGlyphIdMap;
};

#endif
