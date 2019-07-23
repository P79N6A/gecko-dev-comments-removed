








































#include "nsIDeviceContext.h"
#include "nsUnitConversion.h"
#include "nsSystemFontsOS2.h"
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

    
    
    BOOL rc = PrfQueryProfileData(HINI_USER, "PM_SystemFonts", fontType,
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

    char *szFacename = strchr(szFontNameSize, '.') + 1;
    if (!szFacename || (*(szFacename++) == '\0'))
        return NS_ERROR_FAILURE;

    
    aFontStyle->size = atof(szFontNameSize);

    
    
    gfxFloat vertScreenRes = 120; 
    HPS ps = WinGetScreenPS(HWND_DESKTOP);
    HDC dc = GpiQueryDevice(ps);
    LONG lStart = CAPS_FAMILY, lCount = CAPS_CHAR_WIDTH;
    LONG alArray[CAPS_CHAR_WIDTH];
    if (DevQueryCaps(dc, lStart, lCount, alArray)) {
        
        vertScreenRes = alArray[CAPS_VERTICAL_RESOLUTION] * 0.0254;
    }
    WinReleasePS(ps);

    
    aFontStyle->size *= vertScreenRes / 72.0;

    NS_NAMED_LITERAL_STRING(quote, "\""); 
    NS_ConvertUTF8toUTF16 fontFace(szFacename);
    *aFontName = quote + fontFace + quote;

    
    aFontStyle->style = FONT_STYLE_NORMAL;
    aFontStyle->weight = FONT_WEIGHT_NORMAL;

    aFontStyle->systemFont = PR_TRUE;

    return NS_OK;
}
