





































#ifndef GFX_OS2_FONTS_H
#define GFX_OS2_FONTS_H

#include "gfxTypes.h"
#include "gfxFont.h"

#define INCL_GPI
#include <os2.h>
#include <cairo-os2.h>

#include "nsAutoBuffer.h"
#include "nsICharsetConverterManager.h"

class gfxOS2Font : public gfxFont {

public:
    gfxOS2Font(const nsAString &aName, const gfxFontStyle *aFontStyle);
    virtual ~gfxOS2Font();

    virtual const gfxFont::Metrics& GetMetrics();
    double GetWidth(HPS aPS, const char* aString, PRUint32 aLength);
    double GetWidth(HPS aPS, const PRUnichar* aString, PRUint32 aLength);

protected:
    
    

private:
    void ComputeMetrics();

    
    gfxFont::Metrics *mMetrics;
    
};


class THEBES_API gfxOS2FontGroup : public gfxFontGroup {

public:
    gfxOS2FontGroup(const nsAString& aFamilies, const gfxFontStyle* aStyle);
    virtual ~gfxOS2FontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle) {
        NS_ERROR("NOT IMPLEMENTED");
        return nsnull;
    }
    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength, Parameters* aParams);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength, Parameters* aParams);

    const nsACString& GetGenericFamily() const {
        return mGenericFamily;
    }

    PRUint32 FontListLength() const {
        return mFonts.Length();
    }

    gfxOS2Font *GetFontAt(PRInt32 i) {
        return NS_STATIC_CAST(gfxOS2Font*, NS_STATIC_CAST(gfxFont*, mFonts[i]));
    }

    void AppendFont(gfxOS2Font *aFont) {
        mFonts.AppendElement(aFont);
    }

    PRBool HasFontNamed(const nsAString& aName) const {
        PRUint32 len = mFonts.Length();
        for (PRUint32 i = 0; i < len; ++i)
            if (aName.Equals(mFonts[i]->GetName()))
                return PR_TRUE;
        return PR_FALSE;
    }

protected:
    static PRBool MakeFont(const nsAString& fontName,
                           const nsACString& genericName,
                           void *closure);

private:
    friend class gfxOS2TextRun;

    nsCString mGenericFamily;
};


class THEBES_API gfxOS2TextRun {

public:
    gfxOS2TextRun(const nsAString& aString, gfxOS2FontGroup *aFontGroup);
    gfxOS2TextRun(const nsACString& aString, gfxOS2FontGroup *aFontGroup);
    ~gfxOS2TextRun();

    virtual void Draw(gfxContext *aContext, gfxPoint pt);
    virtual gfxFloat Measure(gfxContext *aContext);
    virtual void SetSpacing(const nsTArray<gfxFloat>& spacingArray);
    virtual const nsTArray<gfxFloat> *const GetSpacing() const;
    void SetRightToLeft(PRBool aIsRTL) { mIsRTL = aIsRTL; }
    PRBool IsRightToLeft() { return mIsRTL; }

protected:
private:
    

    double MeasureOrDrawFast(gfxContext *aContext, PRBool aDraw, gfxPoint aPt);
    double MeasureOrDrawSlow(gfxContext *aContext, PRBool aDraw, gfxPoint aPt);

    gfxOS2FontGroup *mGroup;

    nsString mString;
    nsCString mCString;

    const PRPackedBool mIsASCII;
    PRPackedBool mIsRTL;
    nsTArray<gfxFloat> mSpacing;
    double mLength; 
};



enum ConverterRequest {
    eConv_Encoder,
    eConv_Decoder
};

#define CHAR_BUFFER_SIZE 1024
typedef nsAutoBuffer<char, CHAR_BUFFER_SIZE> nsAutoCharBuffer;
typedef nsAutoBuffer<PRUnichar, CHAR_BUFFER_SIZE> nsAutoChar16Buffer;

class gfxOS2Uni {
public:
    static nsISupports* GetUconvObject(int CodePage, ConverterRequest aReq);
    static void FreeUconvObjects();
private:
    static nsICharsetConverterManager* gCharsetManager;
};

nsresult WideCharToMultiByte(int aCodePage,
                             const PRUnichar* aSrc, PRInt32 aSrcLength,
                             nsAutoCharBuffer& aResult, PRInt32& aResultLength);
nsresult MultiByteToWideChar(int aCodePage,
                             const char* aSrc, PRInt32 aSrcLength,
                             nsAutoChar16Buffer& aResult, PRInt32& aResultLength);



#endif 
