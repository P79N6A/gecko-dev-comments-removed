




































#ifndef GFX_DWRITETEXTANALYSIS_H
#define GFX_DWRITETEXTANALYSIS_H

#include "gfxDWriteCommon.h"


class TextAnalysis
    :   public IDWriteTextAnalysisSource,
        public IDWriteTextAnalysisSink        
{
public:

    
    IFACEMETHOD(QueryInterface)(IID const& iid, OUT void** ppObject)
    {
        if (iid == __uuidof(IDWriteTextAnalysisSource)) {
            *ppObject = static_cast<IDWriteTextAnalysisSource*>(this);
            return S_OK;
        } else if (iid == __uuidof(IDWriteTextAnalysisSink)) {
            *ppObject = static_cast<IDWriteTextAnalysisSink*>(this);
            return S_OK;
        } else if (iid == __uuidof(IUnknown)) {
            *ppObject = 
                static_cast<IUnknown*>(static_cast<IDWriteTextAnalysisSource*>(this));
            return S_OK;
        } else {
            return E_NOINTERFACE;
        }
    }

    IFACEMETHOD_(ULONG, AddRef)()
    {
        return 1;
    }

    IFACEMETHOD_(ULONG, Release)()
    {
        return 1;
    }

    
    
    struct Run
    {
        UINT32 mTextStart;   
        UINT32 mTextLength;  
        UINT32 mGlyphStart;  
        UINT32 mGlyphCount;  
                             
        DWRITE_SCRIPT_ANALYSIS mScript;
        UINT8 mBidiLevel;
        bool mIsSideways;

        inline bool ContainsTextPosition(UINT32 aTextPosition) const
        {
            return aTextPosition >= mTextStart
                && aTextPosition <  mTextStart + mTextLength;
        }

        Run *nextRun;
    };

public:
    TextAnalysis(const wchar_t* text,
                 UINT32 textLength,
                 const wchar_t* localeName,
                 DWRITE_READING_DIRECTION readingDirection);

    ~TextAnalysis();

    STDMETHODIMP GenerateResults(IDWriteTextAnalyzer* textAnalyzer,
                                 Run **runHead);

    

    IFACEMETHODIMP GetTextAtPosition(UINT32 textPosition,
                                     OUT WCHAR const** textString,
                                     OUT UINT32* textLength);

    IFACEMETHODIMP GetTextBeforePosition(UINT32 textPosition,
                                         OUT WCHAR const** textString,
                                         OUT UINT32* textLength);

    IFACEMETHODIMP_(DWRITE_READING_DIRECTION) 
        GetParagraphReadingDirection() throw();

    IFACEMETHODIMP GetLocaleName(UINT32 textPosition,
                                 OUT UINT32* textLength,
                                 OUT WCHAR const** localeName);

    IFACEMETHODIMP 
        GetNumberSubstitution(UINT32 textPosition,
                              OUT UINT32* textLength,
                              OUT IDWriteNumberSubstitution** numberSubstitution);

    

    IFACEMETHODIMP 
        SetScriptAnalysis(UINT32 textPosition,
                          UINT32 textLength,
                          DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis);

    IFACEMETHODIMP 
        SetLineBreakpoints(UINT32 textPosition,
                           UINT32 textLength,
                           const DWRITE_LINE_BREAKPOINT* lineBreakpoints);

    IFACEMETHODIMP SetBidiLevel(UINT32 textPosition,
                                UINT32 textLength,
                                UINT8 explicitLevel,
                                UINT8 resolvedLevel);

    IFACEMETHODIMP 
        SetNumberSubstitution(UINT32 textPosition,
                              UINT32 textLength,
                              IDWriteNumberSubstitution* numberSubstitution);

protected:
    Run *FetchNextRun(IN OUT UINT32* textLength);

    void SetCurrentRun(UINT32 textPosition);

    void SplitCurrentRun(UINT32 splitPosition);

protected:
    
    
    
    UINT32 mTextLength;
    const wchar_t* mText;
    const wchar_t* mLocaleName;
    DWRITE_READING_DIRECTION mReadingDirection;

    
    Run *mCurrentRun;

    
    Run  mRunHead;
};

#endif 
