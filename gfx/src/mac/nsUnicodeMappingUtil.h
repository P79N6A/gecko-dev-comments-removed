



































#include <Script.h>

#include "prtypes.h"
#include "nsUnicodeBlock.h"
#include "nsDebug.h"
#include "nscore.h"
#include "nsIPref.h"

class nsUnicodeFontMappingCache;
class nsString;

 
typedef enum {
 kSerif  = 0,
 kSansSerif,
 kMonospace,
 kCursive,
 kFantasy, 
 kUnknownGenericFontName
} nsGenericFontNameType;


class nsUnicodeMappingUtil {
public:
	nsUnicodeMappingUtil();
	~nsUnicodeMappingUtil();
	void Init();
	void CleanUp();
	void Reset();
	inline ScriptCode BlockToScript(nsUnicodeBlock blockID) 
		{ 
			NS_PRECONDITION(blockID < kUnicodeBlockSize, "illegal value");
			return (ScriptCode) mBlockToScriptMapping[blockID]; 
		};
	inline short ScriptFont(ScriptCode script) 
		{ 
			NS_PRECONDITION(script < smPseudoTotalScripts, "bad script code");
			return  mScriptFontMapping[script]; 
		};		
	nsGenericFontNameType MapGenericFontNameType(const nsString& aGenericName);
	inline nsString* GenericFontNameForScript(ScriptCode aScript, nsGenericFontNameType aType) const 
	{
			NS_PRECONDITION(aScript < smPseudoTotalScripts, "bad script code");
			NS_PRECONDITION(aType <= kUnknownGenericFontName, "illegal value");
			if( aType >= kUnknownGenericFontName)
				return nsnull;
			else
				return mGenericFontMapping[aScript][aType]; 
	}
	
  ScriptCode MapLangGroupToScriptCode(const char* aLangGroup);
	static nsUnicodeMappingUtil* GetSingleton();
	static void FreeSingleton();
	nsString *mGenericFontMapping[smPseudoTotalScripts][kUnknownGenericFontName];
	
protected:
    void InitGenericFontMapping();
    void InitBlockToScriptMapping();
    void InitScriptFontMapping();
    void InitFromPref();
   
  static int  PR_CALLBACK_DECL PrefChangedCallback( const char* aPrefName, void* instance_data);
  static void PR_CALLBACK_DECL PrefEnumCallback(const char* aName, void* aClosure);
    
private:
	short 	 mScriptFontMapping[smPseudoTotalScripts];
	PRInt8   mBlockToScriptMapping[kUnicodeBlockSize];
	nsCOMPtr<nsIPref> mPref;
	
	static nsUnicodeMappingUtil* gSingleton;

};
