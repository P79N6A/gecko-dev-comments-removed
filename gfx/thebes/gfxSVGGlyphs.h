



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
#include "nsRefreshDriver.h"
 
class gfxSVGGlyphs;










class gfxSVGGlyphsDocument MOZ_FINAL : public nsAPostRefreshObserver
{
    typedef mozilla::dom::Element Element;

public:
    gfxSVGGlyphsDocument(const uint8_t *aBuffer, uint32_t aBufLen,
                         gfxSVGGlyphs *aSVGGlyphs);

    Element *GetGlyphElement(uint32_t aGlyphId);

    ~gfxSVGGlyphsDocument();

    virtual void DidRefresh() MOZ_OVERRIDE;

private:
    nsresult ParseDocument(const uint8_t *aBuffer, uint32_t aBufLen);

    nsresult SetupPresentation();

    void FindGlyphElements(Element *aElement);

    void InsertGlyphId(Element *aGlyphElement);

    
    gfxSVGGlyphs* mOwner;
    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsIContentViewer> mViewer;
    nsCOMPtr<nsIPresShell> mPresShell;

    nsBaseHashtable<nsUint32HashKey, Element*, Element*> mGlyphIdMap;

    nsAutoCString mSVGGlyphsDocumentURI;
};







class gfxSVGGlyphs
{
private:
    typedef mozilla::dom::Element Element;

public:
    






    gfxSVGGlyphs(hb_blob_t *aSVGTable, gfxFontEntry *aFontEntry);

    


    ~gfxSVGGlyphs();

    


    void DidRefresh();

    




    gfxSVGGlyphsDocument *FindOrCreateGlyphsDocument(uint32_t aGlyphId);

    


    bool HasSVGGlyph(uint32_t aGlyphId);

    





    bool RenderGlyph(gfxContext *aContext, uint32_t aGlyphId, DrawMode aDrawMode,
                     gfxTextContextPaint *aContextPaint);

    




    bool GetGlyphExtents(uint32_t aGlyphId, const gfxMatrix& aSVGToAppSpace,
                         gfxRect *aResult);

private:
    Element *GetGlyphElement(uint32_t aGlyphId);

    nsClassHashtable<nsUint32HashKey, gfxSVGGlyphsDocument> mGlyphDocs;
    nsBaseHashtable<nsUint32HashKey, Element*, Element*> mGlyphIdMap;

    hb_blob_t *mSVGData;
    gfxFontEntry *mFontEntry;

    const struct Header {
        mozilla::AutoSwap_PRUint16 mVersion;
        mozilla::AutoSwap_PRUint32 mDocIndexOffset;
        mozilla::AutoSwap_PRUint32 mColorPalettesOffset;
    } *mHeader;

    struct IndexEntry {
        mozilla::AutoSwap_PRUint16 mStartGlyph;
        mozilla::AutoSwap_PRUint16 mEndGlyph;
        mozilla::AutoSwap_PRUint32 mDocOffset;
        mozilla::AutoSwap_PRUint32 mDocLength;
    };

    const struct DocIndex {
      mozilla::AutoSwap_PRUint16 mNumEntries;
      IndexEntry mEntries[1]; 
    } *mDocIndex;

    static int CompareIndexEntries(const void *_a, const void *_b);
};




class gfxTextContextPaint
{
protected:
    gfxTextContextPaint() { }

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

    virtual ~gfxTextContextPaint() { }

private:
    FallibleTArray<gfxFloat> mDashes;
    gfxFloat mDashOffset;
    gfxFloat mStrokeWidth;
};





class SimpleTextContextPaint : public gfxTextContextPaint
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

    SimpleTextContextPaint(gfxPattern *aFillPattern, gfxPattern *aStrokePattern,
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
