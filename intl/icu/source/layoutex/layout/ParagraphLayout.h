






#ifndef __PARAGRAPHLAYOUT_H

#define __PARAGRAPHLAYOUT_H










#include "unicode/uscript.h"
#if ! UCONFIG_NO_BREAK_ITERATION

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"
#include "layout/LayoutEngine.h"
#include "unicode/ubidi.h"
#include "unicode/brkiter.h"

#include "layout/RunArrays.h"

U_NAMESPACE_BEGIN


















class U_LAYOUTEX_API ParagraphLayout : public UObject
{
public:
    class VisualRun;

    










    class U_LAYOUTEX_API Line : public UObject
    {
    public:
        







        ~Line();

        






        inline le_int32 countRuns() const;

        







        le_int32 getAscent() const;

        







        le_int32 getDescent() const;

        







        le_int32 getLeading() const;

        








        le_int32 getWidth() const;
    
        














        const VisualRun *getVisualRun(le_int32 runIndex) const;

        




        static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

        




        virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

    private:

        



        static const char fgClassID;

        friend class ParagraphLayout;

        le_int32 fAscent;
        le_int32 fDescent;
        le_int32 fLeading;

        le_int32 fRunCount;
        le_int32 fRunCapacity;

        VisualRun **fRuns;

        inline Line();
        inline Line(const Line &other);
        inline Line &operator=(const Line & ) { return *this; };

        void computeMetrics();

        void append(const LEFontInstance *font, UBiDiDirection direction, le_int32 glyphCount,
                    const LEGlyphID glyphs[], const float positions[], const le_int32 glyphToCharMap[]);
    };

    














    class U_LAYOUTEX_API VisualRun : public UObject
    {
    public:
        











        inline const LEFontInstance *getFont() const;

        







        inline UBiDiDirection getDirection() const;

        






        inline le_int32 getGlyphCount() const;

        









        inline const LEGlyphID *getGlyphs() const;

        












        inline const float *getPositions() const;

        









        inline const le_int32 *getGlyphToCharMap() const;

        







        inline le_int32 getAscent() const;

        







        inline le_int32 getDescent() const;

        







        inline le_int32 getLeading() const;

        




        static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

        




        virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

    private:

        



        static const char fgClassID;

        const LEFontInstance *fFont;
        const UBiDiDirection  fDirection;

        const le_int32 fGlyphCount;

        const LEGlyphID *fGlyphs;
        const float     *fPositions;
        const le_int32  *fGlyphToCharMap;

        friend class Line;

        inline VisualRun();
        inline VisualRun(const VisualRun &other);
        inline VisualRun &operator=(const VisualRun &) { return *this; };

        inline VisualRun(const LEFontInstance *font, UBiDiDirection direction, le_int32 glyphCount,
                  const LEGlyphID glyphs[], const float positions[], const le_int32 glyphToCharMap[]);

        ~VisualRun();
    };

    










































    ParagraphLayout(const LEUnicode chars[], le_int32 count,
                    const FontRuns *fontRuns,
                    const ValueRuns *levelRuns,
                    const ValueRuns *scriptRuns,
                    const LocaleRuns *localeRuns,
                    UBiDiLevel paragraphLevel, le_bool vertical,
                    LEErrorCode &status);

    





    ~ParagraphLayout();

    
    
    
#if 0
    














    static le_bool isComplex(const LEUnicode chars[], le_int32 count, const FontRuns *fontRuns);
#else
    











    static le_bool isComplex(const LEUnicode chars[], le_int32 count);

#endif

    








    inline UBiDiLevel getParagraphLevel();

    








    inline UBiDiDirection getTextDirection();

    







    virtual le_int32 getAscent() const;

    







    virtual le_int32 getDescent() const;

    







    virtual le_int32 getLeading() const;

    





    inline void reflow();

#ifndef U_HIDE_INTERNAL_API
    








    inline le_bool isDone() const;
#endif  

    
















    Line *nextLine(float width);

    




    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    




    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

private:


    



    static const char fgClassID;

    struct StyleRunInfo
    {
          LayoutEngine   *engine;
    const LEFontInstance *font;
    const Locale         *locale;
          LEGlyphID      *glyphs;
          float          *positions;
          UScriptCode     script;
          UBiDiLevel      level;
          le_int32        runBase;
          le_int32        runLimit;
          le_int32        glyphBase;
          le_int32        glyphCount;
    };

    ParagraphLayout() {};
    ParagraphLayout(const ParagraphLayout & ) : UObject( ){};
    inline ParagraphLayout &operator=(const ParagraphLayout & ) { return *this; };

    void computeLevels(UBiDiLevel paragraphLevel);

    Line *computeVisualRuns();
    void appendRun(Line *line, le_int32 run, le_int32 firstChar, le_int32 lastChar);

    void computeScripts();

    void computeLocales();

    void computeSubFonts(const FontRuns *fontRuns, LEErrorCode &status);

    void computeMetrics();

    le_int32 getLanguageCode(const Locale *locale);

    le_int32 getCharRun(le_int32 charIndex);

    static le_bool isComplex(UScriptCode script);

    le_int32 previousBreak(le_int32 charIndex);


    const LEUnicode *fChars;
          le_int32   fCharCount;

    const FontRuns   *fFontRuns;
    const ValueRuns  *fLevelRuns;
    const ValueRuns  *fScriptRuns;
    const LocaleRuns *fLocaleRuns;

          le_bool fVertical;
          le_bool fClientLevels;
          le_bool fClientScripts;
          le_bool fClientLocales;

          UBiDiLevel *fEmbeddingLevels;

          le_int32 fAscent;
          le_int32 fDescent;
          le_int32 fLeading;

          le_int32 *fGlyphToCharMap;
          le_int32 *fCharToMinGlyphMap;
          le_int32 *fCharToMaxGlyphMap;
          float    *fGlyphWidths;
          le_int32  fGlyphCount;

          UBiDi *fParaBidi;
          UBiDi *fLineBidi;

          le_int32     *fStyleRunLimits;
          le_int32     *fStyleIndices;
          StyleRunInfo *fStyleRunInfo;
          le_int32      fStyleRunCount;

          BreakIterator *fBreakIterator;
          le_int32       fLineStart;
          le_int32       fLineEnd;

          le_int32       fFirstVisualRun;
          le_int32       fLastVisualRun;
          float          fVisualRunLastX;
          float          fVisualRunLastY;
};

inline UBiDiLevel ParagraphLayout::getParagraphLevel()
{
    return ubidi_getParaLevel(fParaBidi);
}

inline UBiDiDirection ParagraphLayout::getTextDirection()
{
    return ubidi_getDirection(fParaBidi);
}

inline void ParagraphLayout::reflow()
{
    fLineEnd = 0;
}

inline ParagraphLayout::Line::Line()
    : UObject(), fAscent(0), fDescent(0), fLeading(0), fRunCount(0), fRunCapacity(0), fRuns(NULL)
{
    
}

inline ParagraphLayout::Line::Line(const Line & )
    : UObject(), fAscent(0), fDescent(0), fLeading(0), fRunCount(0), fRunCapacity(0), fRuns(NULL)
{
    
}

inline le_int32 ParagraphLayout::Line::countRuns() const
{
    return fRunCount;
}

inline const LEFontInstance *ParagraphLayout::VisualRun::getFont() const
{
    return fFont;
}

inline UBiDiDirection ParagraphLayout::VisualRun::getDirection() const
{
    return fDirection;
}

inline le_int32 ParagraphLayout::VisualRun::getGlyphCount() const
{
    return fGlyphCount;
}

inline const LEGlyphID *ParagraphLayout::VisualRun::getGlyphs() const
{
    return fGlyphs;
}

inline const float *ParagraphLayout::VisualRun::getPositions() const
{
    return fPositions;
}

inline const le_int32 *ParagraphLayout::VisualRun::getGlyphToCharMap() const
{
    return fGlyphToCharMap;
}

inline le_int32 ParagraphLayout::VisualRun::getAscent() const
{
    return fFont->getAscent();
}

inline le_int32 ParagraphLayout::VisualRun::getDescent() const
{
    return fFont->getDescent();
}

inline le_int32 ParagraphLayout::VisualRun::getLeading() const
{
    return fFont->getLeading();
}

inline ParagraphLayout::VisualRun::VisualRun()
    : UObject(), fFont(NULL), fDirection(UBIDI_LTR), fGlyphCount(0), fGlyphs(NULL), fPositions(NULL), fGlyphToCharMap(NULL)
{
    
}

inline ParagraphLayout::VisualRun::VisualRun(const VisualRun &)
    : UObject(), fFont(NULL), fDirection(UBIDI_LTR), fGlyphCount(0), fGlyphs(NULL), fPositions(NULL), fGlyphToCharMap(NULL)
{
    
}

inline ParagraphLayout::VisualRun::VisualRun(const LEFontInstance *font, UBiDiDirection direction, le_int32 glyphCount,
                                             const LEGlyphID glyphs[], const float positions[], const le_int32 glyphToCharMap[])
    : fFont(font), fDirection(direction), fGlyphCount(glyphCount),
      fGlyphs(glyphs), fPositions(positions), fGlyphToCharMap(glyphToCharMap)
{
    
}

U_NAMESPACE_END
#endif
#endif
