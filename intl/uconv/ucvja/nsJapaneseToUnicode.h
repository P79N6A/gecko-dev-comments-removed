



































#ifndef nsShiftJISToUnicode_h__
#define nsShiftJISToUnicode_h__
#include "nsISupports.h"
#include "nsUCSupport.h"


class nsJapaneseToUnicode : public nsBasicDecoderSupport
{
protected:

 void setMapMode();

protected:
 const PRUint16 * const *mMapIndex;
};

class nsShiftJISToUnicode : public nsJapaneseToUnicode
{
public:

 nsShiftJISToUnicode() 
     { 
         mState=0; mData=0; 
         setMapMode();
     };
 virtual ~nsShiftJISToUnicode() {};

 NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;
 NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength) 
     {
        *aDestLength = aSrcLength;
        return NS_OK;
     };
 NS_IMETHOD Reset()
     {
        mState = 0;
        setMapMode();
        return NS_OK;
     };

private:

private:
 PRInt32  mState;
 PRInt32 mData;
};

class nsEUCJPToUnicodeV2 : public nsJapaneseToUnicode
{
public:

 nsEUCJPToUnicodeV2() 
     { 
          mState=0; mData=0; 
          setMapMode();
     };
 virtual ~nsEUCJPToUnicodeV2() {};

 NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;
 NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength) 
     {
        *aDestLength = aSrcLength;
        return NS_OK;
     };
 NS_IMETHOD Reset()
     {
        mState = 0;
        setMapMode();
        return NS_OK;
     };

private:
 PRInt32  mState;
 PRInt32 mData;
};
 
class nsISO2022JPToUnicodeV2 : public nsJapaneseToUnicode
{
public:

 nsISO2022JPToUnicodeV2() 
     { 
        mState = mState_ASCII;
        mLastLegalState = mState_ASCII;
        mData = 0;
        G2charset = G2_unknown;
        mGB2312Decoder = nsnull;
        mEUCKRDecoder = nsnull;
        mISO88597Decoder = nsnull;
        setMapMode();
     };
 virtual ~nsISO2022JPToUnicodeV2()
     {
        NS_IF_RELEASE(mGB2312Decoder);
        NS_IF_RELEASE(mEUCKRDecoder);
        NS_IF_RELEASE(mISO88597Decoder);
     };

 NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;
 NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength) 
     {
        *aDestLength = aSrcLength;
        return NS_OK;
     };
 NS_IMETHOD Reset()
     {
        mState = mState_ASCII;
        mLastLegalState = mState_ASCII;
        setMapMode();
        return NS_OK;
     };

private:
 enum {
   mState_ASCII,
   mState_ESC,
   mState_ESC_28,
   mState_ESC_24,
   mState_ESC_24_28,
   mState_JISX0201_1976Roman,
   mState_JISX0201_1976Kana,
   mState_JISX0208_1978,
   mState_GB2312_1980,
   mState_JISX0208_1983,
   mState_KSC5601_1987,
   mState_JISX0212_1990,
   mState_JISX0208_1978_2ndbyte,
   mState_GB2312_1980_2ndbyte,
   mState_JISX0208_1983_2ndbyte,
   mState_KSC5601_1987_2ndbyte,
   mState_JISX0212_1990_2ndbyte,
   mState_ESC_2e,
   mState_ESC_4e,
   mState_ERROR
 } mState, mLastLegalState;
 PRInt32 mData;
 enum {
   G2_unknown,
   G2_ISO88591,
   G2_ISO88597
 } G2charset;
 nsIUnicodeDecoder *mGB2312Decoder;
 nsIUnicodeDecoder *mEUCKRDecoder;
 nsIUnicodeDecoder *mISO88597Decoder;
};
#endif
