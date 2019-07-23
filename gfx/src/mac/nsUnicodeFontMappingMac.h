




































#ifndef nsUnicodeFontMappingMac_h__
#define nsUnicodeFontMappingMac_h__

#include "nsUnicodeBlock.h"
#include "nsIDeviceContext.h"
#include "nsFont.h"
#include "nsVoidArray.h"
 
#define BAD_FONT_NUM	-1
#define IGNORABLE_FONT_NUM -2
#define BAD_SCRIPT 0x7f

class nsUnicodeMappingUtil;
class nsUnicodeFontMappingCache;

class nsUnicodeFontMappingMac {
public:
   nsUnicodeFontMappingMac(nsFont* aFont, nsIDeviceContext *aDeviceContext, 
   		const nsString& aLangGroup, const nsString& aLANG);
   ~nsUnicodeFontMappingMac();
   		
   short GetFontID(PRUnichar aChar);
   inline const short *GetScriptFallbackFonts() {
   		return mScriptFallbackFontIDs;
   }
   PRBool Equals(const nsUnicodeFontMappingMac& anther);
   
   PRBool ConvertUnicodeToGlyphs(short aFontNum, const PRUnichar* aString,
       ByteCount aStringLength, char *aBuffer, ByteCount aBufferLength, 
       ByteCount& oActualLength, ByteCount& oBytesRead, OptionBits opts);

   static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData);

protected:
   PRBool ScriptMapInitComplete();
   void InitByFontFamily(nsFont* aFont, nsIDeviceContext *aDeviceContext);
   void InitByLANG(const nsString& aLANG);
   void InitByLangGroup(const nsString& aLangGroup);
   void InitDefaultScriptFonts();
   void processOneLangRegion(const char* aLanguage, const char* aRegion );
   nsUnicodeBlock GetBlock(PRUnichar aChar);
private:
   
   PRInt8 mPrivBlockToScript [kUnicodeBlockVarScriptMax] ;
   short  mScriptFallbackFontIDs [smPseudoTotalScripts] ;
   nsAutoVoidArray mFontList;

   static nsUnicodeMappingUtil* gUtil;
};

#endif 
