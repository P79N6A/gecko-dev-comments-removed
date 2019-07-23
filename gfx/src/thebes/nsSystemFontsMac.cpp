




































#include <Carbon/Carbon.h>

#include "nsSystemFontsMac.h"

nsSystemFontsMac::nsSystemFontsMac()
{
}


#define FONTNAME_MAX_UNICHRS sizeof(fontName255) * 2
nsresult 
GetSystemFontForScript(ThemeFontID aFontID, ScriptCode aScriptCode,
                       nsAFlatString& aFontName, SInt16& aFontSize,
                       Style& aFontStyle)
{
    Str255 fontName255;
    ::GetThemeFont(aFontID, aScriptCode, fontName255, &aFontSize, &aFontStyle);
    if (fontName255[0] == 255) {
        NS_WARNING("Too long fong name (> 254 chrs)");
        return NS_ERROR_FAILURE;
    }
    fontName255[fontName255[0]+1] = 0;
        
    OSStatus err;

    
    
    TECObjectRef converter = 0;
    TextEncoding unicodeEncoding = 
        ::CreateTextEncoding(kTextEncodingUnicodeDefault, 
                             kTextEncodingDefaultVariant,
                             kTextEncodingDefaultFormat);
                                                              
    FMFontFamily fontFamily;
    TextEncoding fontEncoding = 0;
    fontFamily = ::FMGetFontFamilyFromName(fontName255);

    err = ::FMGetFontFamilyTextEncoding(fontFamily, &fontEncoding);
    if (err != noErr) {
        NS_WARNING("Could not get the encoding for the font.");
        return NS_ERROR_FAILURE;
    }

    err = ::TECCreateConverter(&converter, fontEncoding, unicodeEncoding);
    if (err != noErr) {
        NS_WARNING("Could not create the converter.");
        return NS_ERROR_FAILURE;
    }

    PRUnichar unicodeFontName[FONTNAME_MAX_UNICHRS + 1];
    ByteCount actualInputLength, actualOutputLength;
    err = ::TECConvertText(converter, &fontName255[1], fontName255[0], 
                           &actualInputLength, 
                           (TextPtr)unicodeFontName,
                           FONTNAME_MAX_UNICHRS * sizeof(PRUnichar),
                           &actualOutputLength);

    if (err != noErr) {
        NS_WARNING("Could not convert the font name.");
        return NS_ERROR_FAILURE;
    }

    ::TECDisposeConverter(converter);

    unicodeFontName[actualOutputLength / sizeof(PRUnichar)] = PRUnichar('\0');
    aFontName = nsDependentString(unicodeFontName);
    return NS_OK;
}

nsresult
nsSystemFontsMac::GetSystemFont(nsSystemFontID aID, nsString *aFontName,
                                gfxFontStyle *aFontStyle) const
{
    nsresult rv;

    
    if (aID == eSystemFont_Window ||
        aID == eSystemFont_Document)
    {
        aFontStyle->style       = FONT_STYLE_NORMAL;
        aFontStyle->weight      = FONT_WEIGHT_NORMAL;

        aFontName->AssignLiteral("sans-serif");
        aFontStyle->size = 14;
        aFontStyle->systemFont = PR_TRUE;

        return NS_OK;
    }

    ThemeFontID fontID = kThemeViewsFont;
    switch (aID) {
        
        case eSystemFont_Caption:       fontID = kThemeSystemFont;         break;
        case eSystemFont_Icon:          fontID = kThemeViewsFont;          break;
        case eSystemFont_Menu:          fontID = kThemeSystemFont;         break;
        case eSystemFont_MessageBox:    fontID = kThemeSmallSystemFont;    break;
        case eSystemFont_SmallCaption:  fontID = kThemeSmallEmphasizedSystemFont;  break;
        case eSystemFont_StatusBar:     fontID = kThemeSmallSystemFont;    break;
        
        
        
        case eSystemFont_Workspace:     fontID = kThemeViewsFont;          break;
        case eSystemFont_Desktop:       fontID = kThemeViewsFont;          break;
        case eSystemFont_Info:          fontID = kThemeViewsFont;          break;
        case eSystemFont_Dialog:        fontID = kThemeSystemFont;         break;
        case eSystemFont_Button:        fontID = kThemeSmallSystemFont;    break;
        case eSystemFont_PullDownMenu:  fontID = kThemeMenuItemFont;       break;
        case eSystemFont_List:          fontID = kThemeSmallSystemFont;    break;
        case eSystemFont_Field:         fontID = kThemeSmallSystemFont;    break;
        
        case eSystemFont_Tooltips:      fontID = kThemeSmallSystemFont;    break;
        case eSystemFont_Widget:        fontID = kThemeSmallSystemFont;    break;
        default:
            
            return NS_ERROR_FAILURE;
    }

    nsAutoString fontName;
    SInt16 fontSize;
    Style fontStyle;

    ScriptCode sysScript = GetScriptManagerVariable(smSysScript);
    rv = GetSystemFontForScript(fontID, smRoman, fontName, fontSize, fontStyle);
    if (NS_FAILED(rv))
        fontName = NS_LITERAL_STRING("Lucida Grande");

    if (sysScript != smRoman) {
        SInt16 localFontSize;
        Style localFontStyle;
        nsAutoString localSysFontName;
        rv = GetSystemFontForScript(fontID, sysScript,
                                    localSysFontName,
                                    localFontSize, localFontStyle);
        if (NS_SUCCEEDED(rv) && !fontName.Equals(localSysFontName)) {
            fontName += NS_LITERAL_STRING(",") + localSysFontName;
            fontSize = localFontSize;
            fontStyle = localFontStyle;
        }
    }

    *aFontName = fontName; 
    aFontStyle->size = gfxFloat(fontSize);

    if (fontStyle & bold)
        aFontStyle->weight = FONT_WEIGHT_BOLD;
    if (fontStyle & italic)
        aFontStyle->style = FONT_STYLE_ITALIC;

    aFontStyle->systemFont = PR_TRUE;

    return NS_OK;
}
