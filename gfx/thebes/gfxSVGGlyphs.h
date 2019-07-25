



































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
#include "mozilla/gfx/UserData.h"






class gfxSVGGlyphs {
private:
    typedef mozilla::dom::Element Element;
    typedef gfxFont::DrawMode DrawMode;

public:
    static const float SVG_UNITS_PER_EM;

    static gfxSVGGlyphs* ParseFromBuffer(uint8_t *aBuffer, uint32_t aBufLen);

    bool HasSVGGlyph(uint32_t aGlyphId);

    bool RenderGlyph(gfxContext *aContext, uint32_t aGlyphId, DrawMode aDrawMode,
                     gfxTextObjectPaint *aObjectPaint);

    bool GetGlyphExtents(uint32_t aGlyphId, const gfxMatrix& aSVGToAppSpace,
                         gfxRect *aResult);

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





class gfxTextObjectPaint
{
protected:
    gfxTextObjectPaint() { }

public:
    static mozilla::gfx::UserDataKey sUserDataKey;

    






    virtual already_AddRefed<gfxPattern> GetFillPattern(float aOpacity = 1.0f) = 0;
    virtual already_AddRefed<gfxPattern> GetStrokePattern(float aOpacity = 1.0f) = 0;

    virtual ~gfxTextObjectPaint() { }
};





class SimpleTextObjectPaint : public gfxTextObjectPaint
{
public:
    SimpleTextObjectPaint(gfxPattern *aFillPattern, gfxPattern *aStrokePattern) :
        mFillPattern(aFillPattern), mStrokePattern(aStrokePattern),
        mFillMatrix(aFillPattern ? aFillPattern->GetMatrix() : gfxMatrix()),
        mStrokeMatrix(aStrokePattern ? aStrokePattern->GetMatrix() : gfxMatrix())
    {
    }

    already_AddRefed<gfxPattern> GetFillPattern(float aOpacity) {
        if (mFillPattern) {
            mFillPattern->SetMatrix(mFillMatrix);
        }
        nsRefPtr<gfxPattern> fillPattern = mFillPattern;
        return fillPattern.forget();
    }

    already_AddRefed<gfxPattern> GetStrokePattern(float aOpacity) {
        if (mStrokePattern) {
            mStrokePattern->SetMatrix(mStrokeMatrix);
        }
        nsRefPtr<gfxPattern> strokePattern = mStrokePattern;
        return strokePattern.forget();
    }

private:
    nsRefPtr<gfxPattern> mFillPattern;
    nsRefPtr<gfxPattern> mStrokePattern;

    gfxMatrix mFillMatrix;
    gfxMatrix mStrokeMatrix;
};

#endif
