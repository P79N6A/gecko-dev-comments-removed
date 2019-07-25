








































#include "nsSystemFontsOS2.h"

#define INCL_WINWINDOWMGR
#define INCL_WINSHELLDATA
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>




static BOOL bIsDBCS;
static BOOL bIsDBCSSet = FALSE;


BOOL IsDBCS()
{
    if (!bIsDBCSSet) {
        
        APIRET rc;
        COUNTRYCODE ctrycodeInfo = {0};
        CHAR        achDBCSInfo[12] = {0};                  
        ctrycodeInfo.country  = 0;                          
        ctrycodeInfo.codepage = 0;                          

        rc = DosQueryDBCSEnv(sizeof(achDBCSInfo), &ctrycodeInfo, achDBCSInfo);
        if (rc == NO_ERROR) {
            
            
            if (achDBCSInfo[0] != 0 || achDBCSInfo[1] != 0 ||
                achDBCSInfo[2] != 0 || achDBCSInfo[3] != 0)
            {
                bIsDBCS = TRUE;
            } else {
                bIsDBCS = FALSE;
            }
        } else {
            bIsDBCS = FALSE;
        } 
        bIsDBCSSet = TRUE;
    } 
    return bIsDBCS;
}


void QueryFontFromINI(char* fontType, char* fontName, ULONG ulLength)
{
    ULONG ulMaxNameL = ulLength;

    
    
    BOOL rc = PrfQueryProfileData(HINI_USER, (PCSZ)"PM_SystemFonts", (PCSZ)fontType,
                                  fontName, &ulMaxNameL);
    
    if (rc == FALSE) {
        
        
        if (!IsDBCS()) {
            strcpy(fontName, "9.WarpSans");
        } else {
            strcpy(fontName, "9.WarpSans Combined");
        }
    } else {
        
        fontName[ulMaxNameL] = '\0';
    }
}



nsSystemFontsOS2::nsSystemFontsOS2()
{
#ifdef DEBUG_thebes
    printf("nsSystemFontsOS2::nsSystemFontsOS2()\n");
#endif
}















nsresult nsSystemFontsOS2::GetSystemFont(nsSystemFontID aID, nsString* aFontName,
                                         gfxFontStyle *aFontStyle) const
{
#ifdef DEBUG_thebes
    printf("nsSystemFontsOS2::GetSystemFont: ");
#endif
    char szFontNameSize[MAXNAMEL];

    switch (aID)
    {
    case eSystemFont_Icon:
        QueryFontFromINI("IconText", szFontNameSize, MAXNAMEL);
#ifdef DEBUG_thebes
        printf("IconText ");
#endif
        break;

    case eSystemFont_Menu:
        QueryFontFromINI("Menus", szFontNameSize, MAXNAMEL);
#ifdef DEBUG_thebes
        printf("Menus ");
#endif
        break;

    case eSystemFont_Caption:
    case eSystemFont_MessageBox:
    case eSystemFont_SmallCaption:
    case eSystemFont_StatusBar:
    case eSystemFont_Tooltips:
    case eSystemFont_Widget:

    case eSystemFont_Window:      
    case eSystemFont_Document:
    case eSystemFont_Workspace:
    case eSystemFont_Desktop:
    case eSystemFont_Info:
    case eSystemFont_Dialog:
    case eSystemFont_Button:
    case eSystemFont_PullDownMenu:
    case eSystemFont_List:
    case eSystemFont_Field:
        QueryFontFromINI("WindowText", szFontNameSize, MAXNAMEL);
#ifdef DEBUG_thebes
        printf("WindowText ");
#endif
        break;

    default:
        NS_WARNING("None of the listed font types, using WarpSans");
        if (!IsDBCS()) {
            strcpy(szFontNameSize, "9.WarpSans");
        } else {
            strcpy(szFontNameSize, "9.WarpSans Combined");
        }
    } 
#ifdef DEBUG_thebes
    printf(" (%s)\n", szFontNameSize);
#endif

    char *szFacename = strchr(szFontNameSize, '.');
    if (!szFacename || (*(szFacename++) == '\0'))
        return NS_ERROR_FAILURE;

    
    aFontStyle->size = atof(szFontNameSize);

    
    
    HPS ps = WinGetScreenPS(HWND_DESKTOP);
    HDC dc = GpiQueryDevice(ps);
    
    LONG vertScreenRes = 120; 
    DevQueryCaps(dc, CAPS_VERTICAL_FONT_RES, 1, &vertScreenRes);
    WinReleasePS(ps);

    
    aFontStyle->size *= vertScreenRes / 72.0;

    NS_ConvertUTF8toUTF16 fontFace(szFacename);
    int pos = 0;

    
    aFontStyle->systemFont = PR_TRUE;

    
    
    NS_NAMED_LITERAL_CSTRING(spcBold, " Bold");
    if ((pos = fontFace.Find(spcBold.get(), PR_FALSE, 0, -1)) > -1) {
        aFontStyle->weight = FONT_WEIGHT_BOLD;
        
        fontFace.Cut(pos, spcBold.Length());
    } else {
        aFontStyle->weight = FONT_WEIGHT_NORMAL;
    }

    
    aFontStyle->stretch = NS_FONT_STRETCH_NORMAL;

    
    NS_NAMED_LITERAL_CSTRING(spcItalic, " Italic");
    NS_NAMED_LITERAL_CSTRING(spcOblique, " Oblique");
    NS_NAMED_LITERAL_CSTRING(spcObli, " Obli");
    if ((pos = fontFace.Find(spcItalic.get(), PR_FALSE, 0, -1)) > -1) {
        aFontStyle->style = FONT_STYLE_ITALIC;
        fontFace.Cut(pos, spcItalic.Length());
    } else if ((pos = fontFace.Find(spcOblique.get(), PR_FALSE, 0, -1)) > -1) {
        
        
        aFontStyle->style = FONT_STYLE_OBLIQUE;
        fontFace.Cut(pos, spcOblique.Length());
    } else if ((pos = fontFace.Find(spcObli.get(), PR_FALSE, 0, -1)) > -1) {
        
        
        aFontStyle->style = FONT_STYLE_OBLIQUE;
        
        
        
        fontFace.Cut(pos, fontFace.Length());
    } else {
        aFontStyle->style = FONT_STYLE_NORMAL;
    }

    
    
    
    if ((pos = fontFace.Find(".", PR_FALSE, 0, -1)) > -1) {
        fontFace.Cut(pos, fontFace.Length());
    }

#ifdef DEBUG_thebes
    printf("  after=%s\n", NS_LossyConvertUTF16toASCII(fontFace).get());
    printf("  style: %s %s %s\n",
           (aFontStyle->weight == FONT_WEIGHT_BOLD) ? "BOLD" : "",
           (aFontStyle->style == FONT_STYLE_ITALIC) ? "ITALIC" : "",
           (aFontStyle->style == FONT_STYLE_OBLIQUE) ? "OBLIQUE" : "");
#endif
    NS_NAMED_LITERAL_STRING(quote, "\""); 
    *aFontName = quote + fontFace + quote;

    return NS_OK;
}
