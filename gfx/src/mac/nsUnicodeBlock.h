



































 
#ifndef nsUnicodeBlock_h__
#define nsUnicodeBlock_h__

#include <Script.h>

enum
{
	smPseudoUnicode			= smUninterp + 0,
	smPseudoUserDef			= smUninterp + 1,
	smPseudoTotalScripts	= smUninterp + 2
};

typedef enum {
 
 kGreek = 0,
 kCyrillic,
 kArmenian,
 kHebrew,
 kArabic, 
 kDevanagari,
 kBengali,
 kGurmukhi,
 kGujarati, 
 kOriya,
 kTamil,
 kTelugu,
 kKannada,
 kMalayalam,
 kThai,
 kLao,
 kTibetan,
 kGeorgian,
 kHangul,
 kBopomofo,
 kEthiopic,
 kKhmer,
 kCanadian,
 kUserDefinedEncoding,
 
 kUnicodeBlockFixedScriptMax,	
 
 
 kBasicLatin = kUnicodeBlockFixedScriptMax,
 kLatin ,
 kCJKMisc,
 kHiraganaKatakana,
 kCJKIdeographs,
 kOthers, 
 
 kUnicodeBlockSize,
 
 kUnicodeBlockVarScriptMax = kUnicodeBlockSize - kUnicodeBlockFixedScriptMax
} nsUnicodeBlock;

#endif nsUnicodeBlock_h__
