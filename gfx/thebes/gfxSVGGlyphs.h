



































#ifndef GFX_SVG_GLYPHS_WRAPPER_H
#define GFX_SVG_GLYPHS_WRAPPER_H

#include "gfxFontUtils.h"
#include "nsString.h"
#include "nsIDocument.h"
#include "nsAutoPtr.h"
#include "nsIContentViewer.h"
#include "nsIPresShell.h"
#include "nsClassHashtable.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "gfxPattern.h"
#include "gfxFont.h"
#include "mozilla/gfx/UserData.h"










class gfxSVGGlyphsDocument
{
    typedef mozilla::dom::Element Element;
    typedef gfxFont::DrawMode DrawMode;

public:
    gfxSVGGlyphsDocument(uint8_t *aBuffer, uint32_t aBufLen,
                         const FallibleTArray<uint8_t>& aCmapTable);

    Element *GetGlyphElement(uint32_t aGlyphId);

    ~gfxSVGGlyphsDocument() {
        if (mViewer) {
            mViewer->Destroy();
        }
    }

private:
    nsresult ParseDocument(uint8_t *aBuffer, uint32_t aBufLen);

    nsresult SetupPresentation();

    void FindGlyphElements(Element *aElement,
                           const FallibleTArray<uint8_t> &aCmapTable);

    void InsertGlyphId(Element *aGlyphElement);
    void InsertGlyphChar(Element *aGlyphElement,
                         const FallibleTArray<uint8_t> &aCmapTable);

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsIContentViewer> mViewer;
    nsCOMPtr<nsIPresShell> mPresShell;

    nsBaseHashtable<nsUint32HashKey, Element*, Element*> mGlyphIdMap;
};







class gfxSVGGlyphs
{
private:
    typedef mozilla::dom::Element Element;
    typedef gfxFont::DrawMode DrawMode;

public:
    static const float SVG_UNITS_PER_EM;

    



    gfxSVGGlyphs(FallibleTArray<uint8_t>& aSVGTable,
                 const FallibleTArray<uint8_t>& aCmapTable);

    


    void UnmangleHeaders();

    




    gfxSVGGlyphsDocument *FindOrCreateGlyphsDocument(uint32_t aGlyphId);

    


    bool HasSVGGlyph(uint32_t aGlyphId);

    





    bool RenderGlyph(gfxContext *aContext, uint32_t aGlyphId, DrawMode aDrawMode,
                     gfxTextObjectPaint *aObjectPaint);

    




    bool GetGlyphExtents(uint32_t aGlyphId, const gfxMatrix& aSVGToAppSpace,
                         gfxRect *aResult);

private:
    Element *GetGlyphElement(uint32_t aGlyphId);

    nsClassHashtable<nsUint32HashKey, gfxSVGGlyphsDocument> mGlyphDocs;
    nsBaseHashtable<nsUint32HashKey, Element*, Element*> mGlyphIdMap;

    FallibleTArray<uint8_t> mSVGData;
    FallibleTArray<uint8_t> mCmapData;

    struct Header {
        uint16_t mVersion;
        uint16_t mIndexLength;
    } *mHeader;

    struct IndexEntry {
        uint16_t mStartGlyph;
        uint16_t mEndGlyph;
        uint32_t mDocOffset;
        uint32_t mDocLength;
    } *mIndex;

    static int CompareIndexEntries(const void *_a, const void *_b);
};





class gfxTextObjectPaint
{
protected:
    gfxTextObjectPaint() { }

public:
    static mozilla::gfx::UserDataKey sUserDataKey;

    




    virtual already_AddRefed<gfxPattern> GetFillPattern(float aOpacity,
                                                        const gfxMatrix& aCTM) = 0;
    virtual already_AddRefed<gfxPattern> GetStrokePattern(float aOpacity,
                                                          const gfxMatrix& aCTM) = 0;

    virtual float GetFillOpacity() { return 1.0f; }
    virtual float GetStrokeOpacity() { return 1.0f; }

    void InitStrokeGeometry(gfxContext *aContext,
                            float devUnitsPerSVGUnit);

    FallibleTArray<gfxFloat>& GetStrokeDashArray() {
        return mDashes;
    }

    gfxFloat GetStrokeDashOffset() {
        return mDashOffset;
    }

    gfxFloat GetStrokeWidth() {
        return mStrokeWidth;
    }

    already_AddRefed<gfxPattern> GetFillPattern(const gfxMatrix& aCTM) {
        return GetFillPattern(GetFillOpacity(), aCTM);
    }

    already_AddRefed<gfxPattern> GetStrokePattern(const gfxMatrix& aCTM) {
        return GetStrokePattern(GetStrokeOpacity(), aCTM);
    }

    virtual ~gfxTextObjectPaint() { }

private:
    FallibleTArray<gfxFloat> mDashes;
    gfxFloat mDashOffset;
    gfxFloat mStrokeWidth;
};





class SimpleTextObjectPaint : public gfxTextObjectPaint
{
private:
    static const gfxRGBA sZero;

public:
    static gfxMatrix SetupDeviceToPatternMatrix(gfxPattern *aPattern,
                                                const gfxMatrix& aCTM)
    {
        if (!aPattern) {
            return gfxMatrix();
        }
        gfxMatrix deviceToUser = aCTM;
        deviceToUser.Invert();
        return deviceToUser * aPattern->GetMatrix();
    }

    SimpleTextObjectPaint(gfxPattern *aFillPattern, gfxPattern *aStrokePattern,
                          const gfxMatrix& aCTM) :
        mFillPattern(aFillPattern ? aFillPattern : new gfxPattern(sZero)),
        mStrokePattern(aStrokePattern ? aStrokePattern : new gfxPattern(sZero))
    {
        mFillMatrix = SetupDeviceToPatternMatrix(aFillPattern, aCTM);
        mStrokeMatrix = SetupDeviceToPatternMatrix(aStrokePattern, aCTM);
    }

    already_AddRefed<gfxPattern> GetFillPattern(float aOpacity,
                                                const gfxMatrix& aCTM) {
        if (mFillPattern) {
            mFillPattern->SetMatrix(aCTM * mFillMatrix);
        }
        nsRefPtr<gfxPattern> fillPattern = mFillPattern;
        return fillPattern.forget();
    }

    already_AddRefed<gfxPattern> GetStrokePattern(float aOpacity,
                                                  const gfxMatrix& aCTM) {
        if (mStrokePattern) {
            mStrokePattern->SetMatrix(aCTM * mStrokeMatrix);
        }
        nsRefPtr<gfxPattern> strokePattern = mStrokePattern;
        return strokePattern.forget();
    }

    float GetFillOpacity() {
        return mFillPattern ? 1.0f : 0.0f;
    }

    float GetStrokeOpacity() {
        return mStrokePattern ? 1.0f : 0.0f;
    }

private:
    nsRefPtr<gfxPattern> mFillPattern;
    nsRefPtr<gfxPattern> mStrokePattern;

    
    gfxMatrix mFillMatrix;
    gfxMatrix mStrokeMatrix;
};

#endif
