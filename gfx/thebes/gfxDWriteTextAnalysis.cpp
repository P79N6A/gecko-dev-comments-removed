




































#include "gfxDWriteTextAnalysis.h"

TextAnalysis::TextAnalysis(const wchar_t* text,
                           UINT32 textLength,
                           const wchar_t* localeName,
                           DWRITE_READING_DIRECTION readingDirection)
  : mText(text)
  , mTextLength(textLength)
  , mLocaleName(localeName)
  , mReadingDirection(readingDirection)
  , mCurrentRun(NULL)
{
}

TextAnalysis::~TextAnalysis()
{
    
    for (Run *run = mRunHead.nextRun; run;) {
        Run *origRun = run;
        run = run->nextRun;
        delete origRun;
    }
}

STDMETHODIMP 
TextAnalysis::GenerateResults(IDWriteTextAnalyzer* textAnalyzer,
                              OUT Run **runHead)
{
    
    

    HRESULT hr = S_OK;

    
    
    mRunHead.mTextStart = 0;
    mRunHead.mTextLength = mTextLength;
    mRunHead.mBidiLevel = 
        (mReadingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
    mRunHead.nextRun = NULL;
    mCurrentRun = &mRunHead;

    
    if (SUCCEEDED(hr = textAnalyzer->AnalyzeScript(this,
                                                   0,
                                                   mTextLength,
                                                   this))) {
        *runHead = &mRunHead;
    }

    return hr;
}





IFACEMETHODIMP 
TextAnalysis::GetTextAtPosition(UINT32 textPosition,
                                OUT WCHAR const** textString,
                                OUT UINT32* textLength)
{
    if (textPosition >= mTextLength) {
        
        *textString = NULL;
        *textLength = 0;
    } else {
        *textString = mText + textPosition;
        *textLength = mTextLength - textPosition;
    }
    return S_OK;
}


IFACEMETHODIMP 
TextAnalysis::GetTextBeforePosition(UINT32 textPosition,
                                    OUT WCHAR const** textString,
                                    OUT UINT32* textLength)
{
    if (textPosition == 0 || textPosition > mTextLength) {
        
        
        *textString = NULL;
        *textLength = 0;
    } else {
        *textString = mText;
        *textLength = textPosition;
    }
    return S_OK;
}


DWRITE_READING_DIRECTION STDMETHODCALLTYPE 
TextAnalysis::GetParagraphReadingDirection()
{
    
    return mReadingDirection;
}


IFACEMETHODIMP 
TextAnalysis::GetLocaleName(UINT32 textPosition,
                            OUT UINT32* textLength,
                            OUT WCHAR const** localeName)
{
    
    *localeName = mLocaleName;
    *textLength = mTextLength - textPosition;

    return S_OK;
}


IFACEMETHODIMP 
TextAnalysis::GetNumberSubstitution(UINT32 textPosition,
                                    OUT UINT32* textLength,
                                    OUT IDWriteNumberSubstitution** numberSubstitution)
{
    
    *numberSubstitution = NULL;
    *textLength = mTextLength - textPosition;

    return S_OK;
}





IFACEMETHODIMP 
TextAnalysis::SetLineBreakpoints(UINT32 textPosition,
                                 UINT32 textLength,
                                 DWRITE_LINE_BREAKPOINT const* lineBreakpoints)
{
    
    return S_OK;
}


IFACEMETHODIMP 
TextAnalysis::SetScriptAnalysis(UINT32 textPosition,
                                UINT32 textLength,
                                DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis)
{
    SetCurrentRun(textPosition);
    SplitCurrentRun(textPosition);
    while (textLength > 0) {
        Run *run = FetchNextRun(&textLength);
        run->mScript = *scriptAnalysis;
    }

    return S_OK;
}


IFACEMETHODIMP 
TextAnalysis::SetBidiLevel(UINT32 textPosition,
                           UINT32 textLength,
                           UINT8 explicitLevel,
                           UINT8 resolvedLevel)
{
    
    return S_OK;
}


IFACEMETHODIMP 
TextAnalysis::SetNumberSubstitution(UINT32 textPosition,
                                    UINT32 textLength,
                                    IDWriteNumberSubstitution* numberSubstitution)
{
    
    return S_OK;
}





TextAnalysis::Run *
TextAnalysis::FetchNextRun(IN OUT UINT32* textLength)
{
    
    
    

    Run *origRun = mCurrentRun;
    
    
    if (*textLength < mCurrentRun->mTextLength) {
        SplitCurrentRun(mCurrentRun->mTextStart + *textLength);
    } else {
        
        mCurrentRun = mCurrentRun->nextRun;
    }
    *textLength -= origRun->mTextLength;

    
    return origRun;
}


void TextAnalysis::SetCurrentRun(UINT32 textPosition)
{
    
    
    
    

    if (mCurrentRun && mCurrentRun->ContainsTextPosition(textPosition)) {
        return;
    }

    for (Run *run = &mRunHead; run; run = run->nextRun) {
        if (run->ContainsTextPosition(textPosition)) {
            mCurrentRun = run;
            return;
        }
    }
    NS_NOTREACHED("We should always be able to find the text position in one \
        of our runs");
}


void TextAnalysis::SplitCurrentRun(UINT32 splitPosition)
{
    if (!mCurrentRun) {
        NS_ASSERTION(false, "SplitCurrentRun called without current run.");
        
        return;
    }
    
    if (splitPosition <= mCurrentRun->mTextStart) {
        
        
        return;
    }
    Run *newRun = new Run;

    *newRun = *mCurrentRun;

    
    newRun->nextRun = mCurrentRun->nextRun;
    mCurrentRun->nextRun = newRun;

    
    UINT32 splitPoint = splitPosition - mCurrentRun->mTextStart;
    newRun->mTextStart += splitPoint;
    newRun->mTextLength -= splitPoint;
    mCurrentRun->mTextLength = splitPoint;
    mCurrentRun = newRun;
}
