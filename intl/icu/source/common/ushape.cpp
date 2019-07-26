

















#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/ushape.h"
#include "cmemory.h"
#include "putilimp.h"
#include "ustr_imp.h"
#include "ubidi_props.h"
#include "uassert.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))





















#define IRRELEVANT 4
#define LAMTYPE    16
#define ALEFTYPE   32
#define LINKR      1
#define LINKL      2
#define APRESENT   8
#define SHADDA     64
#define CSHADDA    128
#define COMBINE    (SHADDA+CSHADDA)

#define HAMZAFE_CHAR       0xfe80
#define HAMZA06_CHAR       0x0621
#define YEH_HAMZA_CHAR     0x0626
#define YEH_HAMZAFE_CHAR   0xFE89
#define LAMALEF_SPACE_SUB  0xFFFF
#define TASHKEEL_SPACE_SUB 0xFFFE
#define NEW_TAIL_CHAR      0xFE73
#define OLD_TAIL_CHAR      0x200B
#define LAM_CHAR           0x0644
#define SPACE_CHAR         0x0020
#define SHADDA_CHAR        0xFE7C
#define TATWEEL_CHAR       0x0640
#define SHADDA_TATWEEL_CHAR  0xFE7D
#define SHADDA06_CHAR      0x0651

#define SHAPE_MODE   0
#define DESHAPE_MODE 1

struct uShapeVariables {
     UChar tailChar;
     uint32_t uShapeLamalefBegin;
     uint32_t uShapeLamalefEnd;
     uint32_t uShapeTashkeelBegin;
     uint32_t uShapeTashkeelEnd;
     int spacesRelativeToTextBeginEnd;
};

static const uint8_t tailFamilyIsolatedFinal[] = {
     1,
     1,
     0,
     0,
     1,
     1,
     0,
     0,
     1,
     1,
     0,
     0,
     1,
     1
};

static const uint8_t tashkeelMedial[] = {
     0,
     1,
     0,
     0,
     0,
     0,
     0,
     1,
     0,
     1,
     0,
     1,
     0,
     1,
     0,
     1
};

static const UChar yehHamzaToYeh[] =
{
 0xFEEF,
 0xFEF0
};

static const uint8_t IrrelevantPos[] = {
    0x0, 0x2, 0x4, 0x6,
    0x8, 0xA, 0xC, 0xE
};


static const UChar convertLamAlef[] =
{
    0x0622,
    0x0622,
    0x0623,
    0x0623,
    0x0625,
    0x0625,
    0x0627,
    0x0627
};

static const UChar araLink[178]=
{
  1           + 32 + 256 * 0x11,
  1           + 32 + 256 * 0x13,
  1                + 256 * 0x15,
  1           + 32 + 256 * 0x17,
  1 + 2            + 256 * 0x19,
  1           + 32 + 256 * 0x1D,
  1 + 2            + 256 * 0x1F,
  1                + 256 * 0x23,
  1 + 2            + 256 * 0x25,
  1 + 2            + 256 * 0x29,
  1 + 2            + 256 * 0x2D,
  1 + 2            + 256 * 0x31,
  1 + 2            + 256 * 0x35,
  1                + 256 * 0x39,
  1                + 256 * 0x3B,
  1                + 256 * 0x3D,
  1                + 256 * 0x3F,
  1 + 2            + 256 * 0x41,
  1 + 2            + 256 * 0x45,
  1 + 2            + 256 * 0x49,
  1 + 2            + 256 * 0x4D,
  1 + 2            + 256 * 0x51,
  1 + 2            + 256 * 0x55,
  1 + 2            + 256 * 0x59,
  1 + 2            + 256 * 0x5D,
  0, 0, 0, 0, 0,                
  1 + 2,                        
  1 + 2            + 256 * 0x61,
  1 + 2            + 256 * 0x65,
  1 + 2            + 256 * 0x69,
  1 + 2       + 16 + 256 * 0x6D,
  1 + 2            + 256 * 0x71,
  1 + 2            + 256 * 0x75,
  1 + 2            + 256 * 0x79,
  1                + 256 * 0x7D,
  1                + 256 * 0x7F,
  1 + 2            + 256 * 0x81,
         4         + 256 * 1,   
         4 + 128   + 256 * 1,   
         4 + 128   + 256 * 1,   
         4 + 128   + 256 * 1,   
         4 + 128   + 256 * 1,   
         4 + 128   + 256 * 1,   
         4 + 64    + 256 * 3,   
         4         + 256 * 1,   
         4         + 256 * 7,   
         4         + 256 * 8,   
         4         + 256 * 8,   
         4         + 256 * 1,   
  0, 0, 0, 0, 0,                
  1                + 256 * 0x85,
  1                + 256 * 0x87,
  1                + 256 * 0x89,
  1                + 256 * 0x8B,
  0, 0, 0, 0, 0,                
  0, 0, 0, 0, 0,                
  0, 0, 0, 0, 0, 0,             
         4         + 256 * 6,   
  1        + 8     + 256 * 0x00,
  1            + 32,            
  1            + 32,            
  0,                            
  1            + 32,            
  1, 1,                         
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, 
  1+2+8+256 * 0x06, 1+2, 1+2, 1+2, 1+2, 1+2, 
  1+2, 1+2, 1+2+8+256 * 0x2A, 1+2,           
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 1+8+256 * 0x3A, 1,       
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, 
  1+2, 1+2, 1+2, 1+2,           
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2+8+256 * 0x3E, 
  1+2, 1+2, 1+2, 1+2,           
  1+2, 1+2+8+256 * 0x42, 1+2, 1+2, 1+2, 1+2, 
  1+2, 1+2, 1+2, 1+2,           
  1+2, 1+2, 1+2, 1+2, 1+2, 1+2, 
  1+2, 1+2,                     
  1,                            
  1+2,                          
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  1+2+8+256 * 0xAC,             
  1,                            
  1+2, 1+2, 1+2, 1+2,           
  1, 1                          
};

static const uint8_t presALink[] = {

    0,    1,    0,    0,    0,    0,    0,    1,    2,1 + 2,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,1 + 2,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    0,    0,    0,    1,
    2,1 + 2,    0,    1,    2,1 + 2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,1 + 2,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    4,    4,
    4,    4,    4
};

static const uint8_t presBLink[]=
{

1 + 2,1 + 2,1 + 2,    0,1 + 2,    0,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,1 + 2,
    0,    0,    1,    0,    1,    0,    1,    0,    1,    0,    1,    2,1 + 2,    0,    1,    0,
    1,    2,1 + 2,    0,    1,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    0,    1,    0,    1,    0,
    1,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,
1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    2,1 + 2,    0,    1,    0,
    1,    0,    1,    2,1 + 2,    0,    1,    0,    1,    0,    1,    0,    1,    0,    0,    0
};

static const UChar convertFBto06[] =
{

   0x671, 0x671,     0,     0,     0,     0, 0x07E, 0x07E, 0x07E, 0x07E,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0, 0x686, 0x686, 0x686, 0x686,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0, 0x698, 0x698,     0,     0, 0x6A9, 0x6A9,
   0x6A9, 0x6A9, 0x6AF, 0x6AF, 0x6AF, 0x6AF,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0, 0x6CC, 0x6CC, 0x6CC, 0x6CC
};

static const UChar convertFEto06[] =
{

   0x64B, 0x64B, 0x64C, 0x64C, 0x64D, 0x64D, 0x64E, 0x64E, 0x64F, 0x64F, 0x650, 0x650, 0x651, 0x651, 0x652, 0x652,
   0x621, 0x622, 0x622, 0x623, 0x623, 0x624, 0x624, 0x625, 0x625, 0x626, 0x626, 0x626, 0x626, 0x627, 0x627, 0x628,
   0x628, 0x628, 0x628, 0x629, 0x629, 0x62A, 0x62A, 0x62A, 0x62A, 0x62B, 0x62B, 0x62B, 0x62B, 0x62C, 0x62C, 0x62C,
   0x62C, 0x62D, 0x62D, 0x62D, 0x62D, 0x62E, 0x62E, 0x62E, 0x62E, 0x62F, 0x62F, 0x630, 0x630, 0x631, 0x631, 0x632,
   0x632, 0x633, 0x633, 0x633, 0x633, 0x634, 0x634, 0x634, 0x634, 0x635, 0x635, 0x635, 0x635, 0x636, 0x636, 0x636,
   0x636, 0x637, 0x637, 0x637, 0x637, 0x638, 0x638, 0x638, 0x638, 0x639, 0x639, 0x639, 0x639, 0x63A, 0x63A, 0x63A,
   0x63A, 0x641, 0x641, 0x641, 0x641, 0x642, 0x642, 0x642, 0x642, 0x643, 0x643, 0x643, 0x643, 0x644, 0x644, 0x644,
   0x644, 0x645, 0x645, 0x645, 0x645, 0x646, 0x646, 0x646, 0x646, 0x647, 0x647, 0x647, 0x647, 0x648, 0x648, 0x649,
   0x649, 0x64A, 0x64A, 0x64A, 0x64A, 0x65C, 0x65C, 0x65D, 0x65D, 0x65E, 0x65E, 0x65F, 0x65F
};

static const uint8_t shapeTable[4][4][4]=
{
  { {0,0,0,0}, {0,0,0,0}, {0,1,0,3}, {0,1,0,1} },
  { {0,0,2,2}, {0,0,1,2}, {0,1,1,2}, {0,1,1,3} },
  { {0,0,0,0}, {0,0,0,0}, {0,1,0,3}, {0,1,0,3} },
  { {0,0,1,2}, {0,0,1,2}, {0,1,1,2}, {0,1,1,3} }
};







static void
_shapeToArabicDigitsWithContext(UChar *s, int32_t length,
                                UChar digitBase,
                                UBool isLogical, UBool lastStrongWasAL) {
    const UBiDiProps *bdp;
    int32_t i;
    UChar c;

    bdp=ubidi_getSingleton();
    digitBase-=0x30;

    
    if(isLogical) {
        for(i=0; i<length; ++i) {
            c=s[i];
            switch(ubidi_getClass(bdp, c)) {
            case U_LEFT_TO_RIGHT: 
            case U_RIGHT_TO_LEFT: 
                lastStrongWasAL=FALSE;
                break;
            case U_RIGHT_TO_LEFT_ARABIC: 
                lastStrongWasAL=TRUE;
                break;
            case U_EUROPEAN_NUMBER: 
                if(lastStrongWasAL && (uint32_t)(c-0x30)<10) {
                    s[i]=(UChar)(digitBase+c); 
                }
                break;
            default :
                break;
            }
        }
    } else {
        for(i=length; i>0; ) {
            c=s[--i];
            switch(ubidi_getClass(bdp, c)) {
            case U_LEFT_TO_RIGHT: 
            case U_RIGHT_TO_LEFT: 
                lastStrongWasAL=FALSE;
                break;
            case U_RIGHT_TO_LEFT_ARABIC: 
                lastStrongWasAL=TRUE;
                break;
            case U_EUROPEAN_NUMBER: 
                if(lastStrongWasAL && (uint32_t)(c-0x30)<10) {
                    s[i]=(UChar)(digitBase+c); 
                }
                break;
            default :
                break;
            }
        }
    }
}







static void
invertBuffer(UChar *buffer, int32_t size, uint32_t , int32_t lowlimit, int32_t highlimit) {
    UChar temp;
    int32_t i=0,j=0;
    for(i=lowlimit,j=size-highlimit-1;i<j;i++,j--) {
        temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }
}









static inline UChar
changeLamAlef(UChar ch) {
    switch(ch) {
    case 0x0622 :
        return 0x065C;
    case 0x0623 :
        return 0x065D;
    case 0x0625 :
        return 0x065E;
    case 0x0627 :
        return 0x065F;
    }
    return 0;
}







static UChar
getLink(UChar ch) {
    if(ch >= 0x0622 && ch <= 0x06D3) {
        return(araLink[ch-0x0622]);
    } else if(ch == 0x200D) {
        return(3);
    } else if(ch >= 0x206D && ch <= 0x206F) {
        return(4);
    }else if(ch >= 0xFB50 && ch <= 0xFC62) {
        return(presALink[ch-0xFB50]);
    } else if(ch >= 0xFE70 && ch <= 0xFEFC) {
        return(presBLink[ch-0xFE70]);
    }else {
        return(0);
    }
}






static void
countSpaces(UChar *dest, int32_t size, uint32_t , int32_t *spacesCountl, int32_t *spacesCountr) {
    int32_t i = 0;
    int32_t countl = 0,countr = 0;
    while((dest[i] == SPACE_CHAR) && (countl < size)) {
       countl++;
       i++;
    }
    if (countl < size) {  
        while(dest[size-1] == SPACE_CHAR) {
            countr++;
            size--;
        }
    }
    *spacesCountl = countl;
    *spacesCountr = countr;
}





static inline int32_t
isTashkeelChar(UChar ch) {
    return (int32_t)( ch>=0x064B && ch<= 0x0652 );
}





static inline int32_t
isTashkeelCharFE(UChar ch) {
    return (int32_t)( ch>=0xFE70 && ch<= 0xFE7F );
}





static inline int32_t
isAlefChar(UChar ch) {
    return (int32_t)( (ch==0x0622)||(ch==0x0623)||(ch==0x0625)||(ch==0x0627) );
}





static inline int32_t
isLamAlefChar(UChar ch) {
    return (int32_t)((ch>=0xFEF5)&&(ch<=0xFEFC) );
}






static inline int32_t
isTailChar(UChar ch) {
    if(ch == OLD_TAIL_CHAR || ch == NEW_TAIL_CHAR){
            return 1;
    }else{
            return 0;
    }
}







static inline int32_t
isSeenTailFamilyChar(UChar ch) {
    if(ch >= 0xfeb1 && ch < 0xfebf){
            return tailFamilyIsolatedFinal [ch - 0xFEB1];
    }else{
            return 0;
    }
}

 




static inline int32_t
isSeenFamilyChar(UChar  ch){
    if(ch >= 0x633 && ch <= 0x636){
        return 1;
    }else {
        return 0;
    }
}







static inline int32_t
isAlefMaksouraChar(UChar ch) {
    return (int32_t)( (ch == 0xFEEF) || ( ch == 0xFEF0) || (ch == 0x0649));
}






static inline int32_t
isYehHamzaChar(UChar ch) {
    if((ch==0xFE89)||(ch==0xFE8A)){
        return 1;
    }else{
        return 0;
    }
}

 






static inline int32_t
isTashkeelOnTatweelChar(UChar ch){
    if(ch >= 0xfe70 && ch <= 0xfe7f && ch != NEW_TAIL_CHAR && ch != 0xFE75 && ch != SHADDA_TATWEEL_CHAR)
    {
        return tashkeelMedial [ch - 0xFE70];
    }else if( (ch >= 0xfcf2 && ch <= 0xfcf4) || (ch == SHADDA_TATWEEL_CHAR)) {
        return 2;
    }else{
        return 0;
    }
}








static inline int32_t
isIsolatedTashkeelChar(UChar ch){
    if(ch >= 0xfe70 && ch <= 0xfe7f && ch != NEW_TAIL_CHAR && ch != 0xFE75){
        return (1 - tashkeelMedial [ch - 0xFE70]);
    }else if(ch >= 0xfc5e && ch <= 0xfc63){
        return 1;
    }else{
        return 0;
    }
}












static int32_t
calculateSize(const UChar *source, int32_t sourceLength,
int32_t destSize,uint32_t options) {
    int32_t i = 0;

    int lamAlefOption = 0;
    int tashkeelOption = 0;

    destSize = sourceLength;

    if (((options&U_SHAPE_LETTERS_MASK) == U_SHAPE_LETTERS_SHAPE ||
        ((options&U_SHAPE_LETTERS_MASK) == U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED )) &&
        ((options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_RESIZE )){
            lamAlefOption = 1;
    }
    if((options&U_SHAPE_LETTERS_MASK) == U_SHAPE_LETTERS_SHAPE &&
       ((options&U_SHAPE_TASHKEEL_MASK) == U_SHAPE_TASHKEEL_RESIZE ) ){
            tashkeelOption = 1;
        }

    if(lamAlefOption || tashkeelOption){
        if((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_VISUAL_LTR) {
            for(i=0;i<sourceLength;i++) {
                if( ((isAlefChar(source[i]))&& (i<(sourceLength-1)) &&(source[i+1] == LAM_CHAR)) || (isTashkeelCharFE(source[i])) ) {
                        destSize--;
                    }
                }
            }else if((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_LOGICAL) {
                for(i=0;i<sourceLength;i++) {
                    if( ( (source[i] == LAM_CHAR) && (i<(sourceLength-1)) && (isAlefChar(source[i+1]))) || (isTashkeelCharFE(source[i])) ) {
                        destSize--;
                    }
                }
            }
        }

    if ((options&U_SHAPE_LETTERS_MASK) == U_SHAPE_LETTERS_UNSHAPE){
        if ( (options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_RESIZE){
            for(i=0;i<sourceLength;i++) {
                if(isLamAlefChar(source[i]))
                destSize++;
            }
        }
    }

    return destSize;
}










static int32_t
handleTashkeelWithTatweel(UChar *dest, int32_t sourceLength,
             int32_t , uint32_t ,
             UErrorCode * ) {
                 int i;
                 for(i = 0; i < sourceLength; i++){
                     if((isTashkeelOnTatweelChar(dest[i]) == 1)){
                         dest[i] = TATWEEL_CHAR;
                    }else if((isTashkeelOnTatweelChar(dest[i]) == 2)){
                         dest[i] = SHADDA_TATWEEL_CHAR;
                    }else if(isIsolatedTashkeelChar(dest[i]) && dest[i] != SHADDA_CHAR){
                         dest[i] = SPACE_CHAR;
                    }
                 }
                 return sourceLength;
}






















static int32_t
handleGeneratedSpaces(UChar *dest, int32_t sourceLength,
                    int32_t destSize,
                    uint32_t options,
                    UErrorCode *pErrorCode,struct uShapeVariables shapeVars ) {

    int32_t i = 0, j = 0;
    int32_t count = 0;
    UChar *tempbuffer=NULL;

    int lamAlefOption = 0;
    int tashkeelOption = 0;
    int shapingMode = SHAPE_MODE;

    if (shapingMode == 0){
        if ( (options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_RESIZE ){
            lamAlefOption = 1;
        }
        if ( (options&U_SHAPE_TASHKEEL_MASK) == U_SHAPE_TASHKEEL_RESIZE ){
            tashkeelOption = 1;
        }
    }

    tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);
    
    if(tempbuffer == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }


    if (lamAlefOption || tashkeelOption){
        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

        i = j = 0; count = 0;
        while(i < sourceLength) {
            if ( (lamAlefOption && dest[i] == LAMALEF_SPACE_SUB) ||
               (tashkeelOption && dest[i] == TASHKEEL_SPACE_SUB) ){
                j--;
                count++;
            } else {
                tempbuffer[j] = dest[i];
            }
            i++;
            j++;
        }

        while(count >= 0) {
            tempbuffer[i] = 0x0000;
            i--;
            count--;
        }

        uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);
        destSize = u_strlen(dest);
    }

      lamAlefOption = 0;

    if (shapingMode == 0){
        if ( (options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_NEAR ){
            lamAlefOption = 1;
        }
    }

    if (lamAlefOption){
        
        i = 0;
        while(i < sourceLength) {
            if(lamAlefOption&&dest[i] == LAMALEF_SPACE_SUB){
                dest[i] = SPACE_CHAR;
            }
            i++;
        }
        destSize = sourceLength;
    }
    lamAlefOption = 0;
    tashkeelOption = 0;

    if (shapingMode == 0) {
        if ( ((options&U_SHAPE_LAMALEF_MASK) == shapeVars.uShapeLamalefBegin) ||
              (((options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_AUTO )
              && (shapeVars.spacesRelativeToTextBeginEnd==1)) ) {
            lamAlefOption = 1;
        }
        if ( (options&U_SHAPE_TASHKEEL_MASK) == shapeVars.uShapeTashkeelBegin ) {
            tashkeelOption = 1;
        }
    }

    if(lamAlefOption || tashkeelOption){
        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);
        
        i = j = sourceLength; count = 0;
        
        while(i >= 0) {
            if ( (lamAlefOption && dest[i] == LAMALEF_SPACE_SUB) ||
                 (tashkeelOption && dest[i] == TASHKEEL_SPACE_SUB) ){
                j++;
                count++;
            }else {
                tempbuffer[j] = dest[i];
            }
            i--;
            j--;
        }

        for(i=0 ;i < count; i++){
                tempbuffer[i] = SPACE_CHAR;
        }

        uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);
        destSize = sourceLength;
    }



    lamAlefOption = 0;
    tashkeelOption = 0;

    if (shapingMode == 0) {
        if ( ((options&U_SHAPE_LAMALEF_MASK) == shapeVars.uShapeLamalefEnd) ||
              (((options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_AUTO )
              && (shapeVars.spacesRelativeToTextBeginEnd==0)) ) {
            lamAlefOption = 1;
        }
        if ( (options&U_SHAPE_TASHKEEL_MASK) == shapeVars.uShapeTashkeelEnd ){
            tashkeelOption = 1;
        }
    }

    if(lamAlefOption || tashkeelOption){
        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

        i = j = 0; count = 0;
        while(i < sourceLength) {
            if ( (lamAlefOption && dest[i] == LAMALEF_SPACE_SUB) ||
                 (tashkeelOption && dest[i] == TASHKEEL_SPACE_SUB) ){
                j--;
                count++;
            }else {
                tempbuffer[j] = dest[i];
            }
            i++;
            j++;
        }

        while(count >= 0) {
            tempbuffer[i] = SPACE_CHAR;
            i--;
            count--;
        }

        uprv_memcpy(dest,tempbuffer, sourceLength*U_SIZEOF_UCHAR);
        destSize = sourceLength;
    }


    if(tempbuffer){
        uprv_free(tempbuffer);
    }

    return destSize;
}











static int32_t
expandCompositCharAtBegin(UChar *dest, int32_t sourceLength, int32_t destSize,UErrorCode *pErrorCode) {
    int32_t      i = 0,j = 0;
    int32_t      countl = 0;
    UChar    *tempbuffer=NULL;

    tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);

    
    if(tempbuffer == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

        uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

        i = 0;
        while(dest[i] == SPACE_CHAR) {
            countl++;
            i++;
        }

        i = j = sourceLength-1;

        while(i >= 0 && j >= 0) {
            if( countl>0 && isLamAlefChar(dest[i])) {
                tempbuffer[j] = LAM_CHAR;
                
                U_ASSERT(dest[i] >= 0xFEF5u
                    && dest[i]-0xFEF5u < sizeof(convertLamAlef)/sizeof(convertLamAlef[0]));
                tempbuffer[j-1] = convertLamAlef[ dest[i] - 0xFEF5 ];
                j--;
                countl--;
            }else {
                 if( countl == 0 && isLamAlefChar(dest[i]) ) {
                     *pErrorCode=U_NO_SPACE_AVAILABLE;
                     }
                 tempbuffer[j] = dest[i];
            }
            i--;
            j--;
        }
        uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);

        uprv_free(tempbuffer);

        destSize = sourceLength;
        return destSize;
}











static int32_t
expandCompositCharAtEnd(UChar *dest, int32_t sourceLength, int32_t destSize,UErrorCode *pErrorCode) {
    int32_t      i = 0,j = 0;

    int32_t      countr = 0;
    int32_t  inpsize = sourceLength;

    UChar    *tempbuffer=NULL;
    tempbuffer = (UChar *)uprv_malloc((sourceLength+1)*U_SIZEOF_UCHAR);

    
    if(tempbuffer == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
         return 0;
    }

    uprv_memset(tempbuffer, 0, (sourceLength+1)*U_SIZEOF_UCHAR);

    while(dest[inpsize-1] == SPACE_CHAR) {
        countr++;
        inpsize--;
    }

    i = sourceLength - countr - 1;
    j = sourceLength - 1;

    while(i >= 0 && j >= 0) {
        if( countr>0 && isLamAlefChar(dest[i]) ) {
            tempbuffer[j] = LAM_CHAR;
            tempbuffer[j-1] = convertLamAlef[ dest[i] - 0xFEF5 ];
            j--;
            countr--;
        }else {
            if ((countr == 0) && isLamAlefChar(dest[i]) ) {
                *pErrorCode=U_NO_SPACE_AVAILABLE;
            }
            tempbuffer[j] = dest[i];
        }
        i--;
        j--;
    }

    if(countr > 0) {
        uprv_memmove(tempbuffer, tempbuffer+countr, sourceLength*U_SIZEOF_UCHAR);
        if(u_strlen(tempbuffer) < sourceLength) {
            for(i=sourceLength-1;i>=sourceLength-countr;i--) {
                tempbuffer[i] = SPACE_CHAR;
            }
        }
    }
    uprv_memcpy(dest, tempbuffer, sourceLength*U_SIZEOF_UCHAR);

    uprv_free(tempbuffer);

    destSize = sourceLength;
    return destSize;
}










static int32_t
expandCompositCharAtNear(UChar *dest, int32_t sourceLength, int32_t destSize,UErrorCode *pErrorCode,
                         int yehHamzaOption, int seenTailOption, int lamAlefOption, struct uShapeVariables shapeVars) {
    int32_t      i = 0;


    UChar    lamalefChar, yehhamzaChar;

    for(i = 0 ;i<=sourceLength-1;i++) {
            if (seenTailOption && isSeenTailFamilyChar(dest[i])) {
                if ((i>0) && (dest[i-1] == SPACE_CHAR) ) {
                    dest[i-1] = shapeVars.tailChar;
                }else {
                    *pErrorCode=U_NO_SPACE_AVAILABLE;
                }
            }else if(yehHamzaOption && (isYehHamzaChar(dest[i])) ) {
                if ((i>0) && (dest[i-1] == SPACE_CHAR) ) {
                    yehhamzaChar = dest[i];
                    dest[i] = yehHamzaToYeh[yehhamzaChar - YEH_HAMZAFE_CHAR];
                    dest[i-1] = HAMZAFE_CHAR;
                }else {

                    *pErrorCode=U_NO_SPACE_AVAILABLE;
                }
            }else if(lamAlefOption && isLamAlefChar(dest[i+1])) {
                if(dest[i] == SPACE_CHAR){
                    lamalefChar = dest[i+1];
                    dest[i+1] = LAM_CHAR;
                    dest[i] = convertLamAlef[ lamalefChar - 0xFEF5 ];
                }else {
                    *pErrorCode=U_NO_SPACE_AVAILABLE;
                }
            }
       }
       destSize = sourceLength;
       return destSize;
}
 















static int32_t
expandCompositChar(UChar *dest, int32_t sourceLength,
              int32_t destSize,uint32_t options,
              UErrorCode *pErrorCode, int shapingMode,struct uShapeVariables shapeVars) {

    int32_t      i = 0,j = 0;

    UChar    *tempbuffer=NULL;
    int yehHamzaOption = 0;
    int seenTailOption = 0;
    int lamAlefOption = 0;

    if (shapingMode == 1){
        if ( (options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_AUTO){

            if(shapeVars.spacesRelativeToTextBeginEnd == 0) {
                destSize = expandCompositCharAtEnd(dest, sourceLength, destSize, pErrorCode);

                if(*pErrorCode == U_NO_SPACE_AVAILABLE) {
                    *pErrorCode = U_ZERO_ERROR;
                    destSize = expandCompositCharAtBegin(dest, sourceLength, destSize, pErrorCode);
                }
            }else {
                destSize = expandCompositCharAtBegin(dest, sourceLength, destSize, pErrorCode);

                if(*pErrorCode == U_NO_SPACE_AVAILABLE) {
                    *pErrorCode = U_ZERO_ERROR;
                    destSize = expandCompositCharAtEnd(dest, sourceLength, destSize, pErrorCode);
                }
            }

            if(*pErrorCode == U_NO_SPACE_AVAILABLE) {
                *pErrorCode = U_ZERO_ERROR;
                destSize = expandCompositCharAtNear(dest, sourceLength, destSize, pErrorCode, yehHamzaOption,
                                                seenTailOption, 1,shapeVars);
            }
        }
    }

    if (shapingMode == 1){
        if ( (options&U_SHAPE_LAMALEF_MASK) == shapeVars.uShapeLamalefEnd){
            destSize = expandCompositCharAtEnd(dest, sourceLength, destSize, pErrorCode);
        }
    }

    if (shapingMode == 1){
        if ( (options&U_SHAPE_LAMALEF_MASK) == shapeVars.uShapeLamalefBegin){
            destSize = expandCompositCharAtBegin(dest, sourceLength, destSize, pErrorCode);
        }
    }

    if (shapingMode == 0){
         if ((options&U_SHAPE_YEHHAMZA_MASK) == U_SHAPE_YEHHAMZA_TWOCELL_NEAR){
             yehHamzaOption = 1;
         }
         if ((options&U_SHAPE_SEEN_MASK) == U_SHAPE_SEEN_TWOCELL_NEAR){
            seenTailOption = 1;
         }
    }
    if (shapingMode == 1) {
        if ( (options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_NEAR) {
            lamAlefOption = 1;
        }
    }


    if (yehHamzaOption || seenTailOption || lamAlefOption){
        destSize = expandCompositCharAtNear(dest, sourceLength, destSize, pErrorCode, yehHamzaOption,
                                            seenTailOption,lamAlefOption,shapeVars);
    }


    if (shapingMode == 1){
        if ( (options&U_SHAPE_LAMALEF_MASK) == U_SHAPE_LAMALEF_RESIZE){
            destSize = calculateSize(dest,sourceLength,destSize,options);
            tempbuffer = (UChar *)uprv_malloc((destSize+1)*U_SIZEOF_UCHAR);

            
            if(tempbuffer == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }

            uprv_memset(tempbuffer, 0, (destSize+1)*U_SIZEOF_UCHAR);

            i = j = 0;
            while(i < destSize && j < destSize) {
                if(isLamAlefChar(dest[i]) ) {
                    tempbuffer[j] = convertLamAlef[ dest[i] - 0xFEF5 ];
                    tempbuffer[j+1] = LAM_CHAR;
                    j++;
                }else {
                    tempbuffer[j] = dest[i];
                }
                i++;
                j++;
            }

            uprv_memcpy(dest, tempbuffer, destSize*U_SIZEOF_UCHAR);
        }
    }

    if(tempbuffer) {
        uprv_free(tempbuffer);
    }
    return destSize;
}






static int32_t
shapeUnicode(UChar *dest, int32_t sourceLength,
             int32_t destSize,uint32_t options,
             UErrorCode *pErrorCode,
             int tashkeelFlag, struct uShapeVariables shapeVars) {

    int32_t          i, iend;
    int32_t          step;
    int32_t          lastPos,Nx, Nw;
    unsigned int     Shape;
    int32_t          lamalef_found = 0;
    int32_t seenfamFound = 0, yehhamzaFound =0, tashkeelFound  = 0;
    UChar            prevLink = 0, lastLink = 0, currLink, nextLink = 0;
    UChar            wLamalef;

    





    if ((options & U_SHAPE_PRESERVE_PRESENTATION_MASK)  == U_SHAPE_PRESERVE_PRESENTATION_NOOP) {
        for (i = 0; i < sourceLength; i++) {
            UChar inputChar  = dest[i];
            if ( (inputChar >= 0xFB50) && (inputChar <= 0xFBFF)) {
                UChar c = convertFBto06 [ (inputChar - 0xFB50) ];
                if (c != 0)
                    dest[i] = c;
            } else if ( (inputChar >= 0xFE70) && (inputChar <= 0xFEFC)) {
                dest[i] = convertFEto06 [ (inputChar - 0xFE70) ] ;
            } else {
                dest[i] = inputChar ;
            }
        }
    }


    
    i = sourceLength - 1;
    iend = -1;
    step = -1;

    




    currLink = getLink(dest[i]);

    lastPos = i;
    Nx = -2, Nw = 0;

    while (i != iend) {
        
        if ((currLink & 0xFF00) > 0 || (getLink(dest[i]) & IRRELEVANT) != 0) {
            Nw = i + step;
            while (Nx < 0) {         
                if(Nw == iend) {
                    nextLink = 0;
                    Nx = 3000;
                } else {
                    nextLink = getLink(dest[Nw]);
                    if((nextLink & IRRELEVANT) == 0) {
                        Nx = Nw;
                    } else {
                        Nw = Nw + step;
                    }
                }
            }

            if ( ((currLink & ALEFTYPE) > 0)  &&  ((lastLink & LAMTYPE) > 0) ) {
                lamalef_found = 1;
                wLamalef = changeLamAlef(dest[i]); 
                if ( wLamalef != 0) {
                    dest[i] = LAMALEF_SPACE_SUB;            
                    dest[lastPos] =wLamalef;     
                    i=lastPos;                   
                }                                
                lastLink = prevLink;             
                currLink = getLink(wLamalef);    
            }                                    

            if ((i > 0) && (dest[i-1] == SPACE_CHAR)){
                if ( isSeenFamilyChar(dest[i])) {
                    seenfamFound = 1;
                } else if (dest[i] == YEH_HAMZA_CHAR) {
                    yehhamzaFound = 1;
                }
            }
            else if(i==0){
                if ( isSeenFamilyChar(dest[i])){
                    seenfamFound = 1;
                } else if (dest[i] == YEH_HAMZA_CHAR) {
                    yehhamzaFound = 1;
                }
            }

            




            Shape = shapeTable[nextLink & (LINKR + LINKL)]
                              [lastLink & (LINKR + LINKL)]
                              [currLink & (LINKR + LINKL)];

            if ((currLink & (LINKR+LINKL)) == 1) {
                Shape &= 1;
            } else if(isTashkeelChar(dest[i])) {
                if( (lastLink & LINKL) && (nextLink & LINKR) && (tashkeelFlag == 1) &&
                     dest[i] != 0x064C && dest[i] != 0x064D )
                {
                    Shape = 1;
                    if( (nextLink&ALEFTYPE) == ALEFTYPE && (lastLink&LAMTYPE) == LAMTYPE ) {
                        Shape = 0;
                    }
                } else if(tashkeelFlag == 2 && dest[i] == SHADDA06_CHAR){
                    Shape = 1;
                } else {
                    Shape = 0;
                }
            }
            if ((dest[i] ^ 0x0600) < 0x100) {
                if ( isTashkeelChar(dest[i]) ){
                    if (tashkeelFlag == 2  && dest[i] != SHADDA06_CHAR){
                        dest[i] = TASHKEEL_SPACE_SUB;
                        tashkeelFound  = 1;
                    } else {
                        
                        U_ASSERT(dest[i] >= 0x064Bu
                            && dest[i]-0x064Bu < sizeof(IrrelevantPos)/sizeof(IrrelevantPos[0]));
                        dest[i] =  0xFE70 + IrrelevantPos[(dest[i] - 0x064B)] + Shape;
                    }
                }else if ((currLink & APRESENT) > 0) {
                    dest[i] = (UChar)(0xFB50 + (currLink >> 8) + Shape);
                }else if ((currLink >> 8) > 0 && (currLink & IRRELEVANT) == 0) {
                    dest[i] = (UChar)(0xFE70 + (currLink >> 8) + Shape);
                }
            }
        }

        
        if ((currLink & IRRELEVANT) == 0) {
            prevLink = lastLink;
            lastLink = currLink;
            lastPos = i;
        }

        i = i + step;
        if (i == Nx) {
            currLink = nextLink;
            Nx = -2;
        } else if(i != iend) {
            currLink = getLink(dest[i]);
        }
    }
    destSize = sourceLength;
    if ( (lamalef_found != 0 ) || (tashkeelFound  != 0) ){
        destSize = handleGeneratedSpaces(dest,sourceLength,destSize,options,pErrorCode, shapeVars);
    }

    if ( (seenfamFound != 0) || (yehhamzaFound != 0) ) {
        destSize = expandCompositChar(dest, sourceLength,destSize,options,pErrorCode, SHAPE_MODE,shapeVars);
    }
    return destSize;
}






static int32_t
deShapeUnicode(UChar *dest, int32_t sourceLength,
               int32_t destSize,uint32_t options,
               UErrorCode *pErrorCode, struct uShapeVariables shapeVars) {
    int32_t i = 0;
    int32_t lamalef_found = 0;
    int32_t yehHamzaComposeEnabled = 0;
    int32_t seenComposeEnabled = 0;

    yehHamzaComposeEnabled = ((options&U_SHAPE_YEHHAMZA_MASK) == U_SHAPE_YEHHAMZA_TWOCELL_NEAR) ? 1 : 0;
    seenComposeEnabled = ((options&U_SHAPE_SEEN_MASK) == U_SHAPE_SEEN_TWOCELL_NEAR)? 1 : 0;

    




    for(i = 0; i < sourceLength; i++) {
        UChar  inputChar = dest[i];
        if ( (inputChar >= 0xFB50) && (inputChar <= 0xFBFF)) { 
            UChar c = convertFBto06 [ (inputChar - 0xFB50) ];
            if (c != 0)
                dest[i] = c;
        } else if( (yehHamzaComposeEnabled == 1) && ((inputChar == HAMZA06_CHAR) || (inputChar == HAMZAFE_CHAR))
                && (i < (sourceLength - 1)) && isAlefMaksouraChar(dest[i+1] )) {
                 dest[i] = SPACE_CHAR;
                 dest[i+1] = YEH_HAMZA_CHAR;
        } else if ( (seenComposeEnabled == 1) && (isTailChar(inputChar)) && (i< (sourceLength - 1))
                        && (isSeenTailFamilyChar(dest[i+1])) ) {
                dest[i] = SPACE_CHAR;
        } else if (( inputChar >= 0xFE70) && (inputChar <= 0xFEF4 )) { 
                dest[i] = convertFEto06 [ (inputChar - 0xFE70) ];
        } else {
            dest[i] = inputChar ;
        }

        if( isLamAlefChar(dest[i]) )
            lamalef_found = 1;
    }

   destSize = sourceLength;
   if (lamalef_found != 0){
          destSize = expandCompositChar(dest,sourceLength,destSize,options,pErrorCode,DESHAPE_MODE, shapeVars);
   }
   return destSize;
}







U_CAPI int32_t U_EXPORT2
u_shapeArabic(const UChar *source, int32_t sourceLength,
              UChar *dest, int32_t destCapacity,
              uint32_t options,
              UErrorCode *pErrorCode) {

    int32_t destLength;
    struct  uShapeVariables shapeVars = { OLD_TAIL_CHAR,U_SHAPE_LAMALEF_BEGIN,U_SHAPE_LAMALEF_END,U_SHAPE_TASHKEEL_BEGIN,U_SHAPE_TASHKEEL_END,0};

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    if( source==NULL || sourceLength<-1 || (dest==NULL && destCapacity!=0) || destCapacity<0 ||
                (((options&U_SHAPE_TASHKEEL_MASK) > 0) &&
                 ((options&U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED) == U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED) ) ||
                (((options&U_SHAPE_TASHKEEL_MASK) > 0) &&
                 ((options&U_SHAPE_LETTERS_MASK) == U_SHAPE_LETTERS_UNSHAPE)) ||
                (options&U_SHAPE_DIGIT_TYPE_RESERVED)==U_SHAPE_DIGIT_TYPE_RESERVED ||
                (options&U_SHAPE_DIGITS_MASK)==U_SHAPE_DIGITS_RESERVED ||
                ((options&U_SHAPE_LAMALEF_MASK) != U_SHAPE_LAMALEF_RESIZE  &&
                (options&U_SHAPE_AGGREGATE_TASHKEEL_MASK) != 0) ||
                ((options&U_SHAPE_AGGREGATE_TASHKEEL_MASK) == U_SHAPE_AGGREGATE_TASHKEEL &&
                (options&U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED) != U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED)
    )
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    if(((options&U_SHAPE_LAMALEF_MASK) > 0)&&
              !(((options & U_SHAPE_LAMALEF_MASK)==U_SHAPE_LAMALEF_BEGIN) ||
                ((options & U_SHAPE_LAMALEF_MASK)==U_SHAPE_LAMALEF_END ) ||
                ((options & U_SHAPE_LAMALEF_MASK)==U_SHAPE_LAMALEF_RESIZE )||
                 ((options & U_SHAPE_LAMALEF_MASK)==U_SHAPE_LAMALEF_AUTO) ||
                 ((options & U_SHAPE_LAMALEF_MASK)==U_SHAPE_LAMALEF_NEAR)))
    {
         *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    if(((options&U_SHAPE_TASHKEEL_MASK) > 0)&&
                   !(((options & U_SHAPE_TASHKEEL_MASK)==U_SHAPE_TASHKEEL_BEGIN) ||
                     ((options & U_SHAPE_TASHKEEL_MASK)==U_SHAPE_TASHKEEL_END )
                    ||((options & U_SHAPE_TASHKEEL_MASK)==U_SHAPE_TASHKEEL_RESIZE )||
                    ((options & U_SHAPE_TASHKEEL_MASK)==U_SHAPE_TASHKEEL_REPLACE_BY_TATWEEL)))
    {
         *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    if(sourceLength==-1) {
        sourceLength=u_strlen(source);
    }
    if(sourceLength<=0) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    }

    
    if( dest!=NULL &&
        ((source<=dest && dest<source+sourceLength) ||
         (dest<=source && source<dest+destCapacity))) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if ( (options&U_SHAPE_TAIL_TYPE_MASK) == U_SHAPE_TAIL_NEW_UNICODE){
        shapeVars.tailChar = NEW_TAIL_CHAR;
    }else {
        shapeVars.tailChar = OLD_TAIL_CHAR;
    }

    if((options&U_SHAPE_LETTERS_MASK)!=U_SHAPE_LETTERS_NOOP) {
        UChar buffer[300];
        UChar *tempbuffer, *tempsource = NULL;
        int32_t outputSize, spacesCountl=0, spacesCountr=0;

        if((options&U_SHAPE_AGGREGATE_TASHKEEL_MASK)>0) {
            int32_t logical_order = (options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_LOGICAL;
            int32_t aggregate_tashkeel =
                        (options&(U_SHAPE_AGGREGATE_TASHKEEL_MASK+U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED)) ==
                        (U_SHAPE_AGGREGATE_TASHKEEL+U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED);
            int step=logical_order?1:-1;
            int j=logical_order?-1:2*sourceLength;
            int i=logical_order?-1:sourceLength;
            int end=logical_order?sourceLength:-1;
            int aggregation_possible = 1;
            UChar prev = 0;
            UChar prevLink, currLink = 0;
            int newSourceLength = 0;
            tempsource = (UChar *)uprv_malloc(2*sourceLength*U_SIZEOF_UCHAR);
            if(tempsource == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }

            while ((i+=step) != end) {
                prevLink = currLink;
                currLink = getLink(source[i]);
                if (aggregate_tashkeel && ((prevLink|currLink)&COMBINE) == COMBINE && aggregation_possible) {
                    aggregation_possible = 0;
                    tempsource[j] = (prev<source[i]?prev:source[i])-0x064C+0xFC5E;
                    currLink = getLink(tempsource[j]);
                } else {
                    aggregation_possible = 1;
                    tempsource[j+=step] = source[i];
                    prev = source[i];
                    newSourceLength++;
                }
            }
            source = tempsource+(logical_order?0:j);
            sourceLength = newSourceLength;
        }

        
        
        if(((options&U_SHAPE_LAMALEF_MASK)==U_SHAPE_LAMALEF_RESIZE) ||
           ((options&U_SHAPE_TASHKEEL_MASK)==U_SHAPE_TASHKEEL_RESIZE)) {
            outputSize=calculateSize(source,sourceLength,destCapacity,options);
        } else {
            outputSize=sourceLength;
        }

        if(outputSize>destCapacity) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                if (tempsource != NULL) uprv_free(tempsource);
            return outputSize;
        }

        



        if(sourceLength>outputSize) {
            outputSize=sourceLength;
        }

        
        if(outputSize<=LENGTHOF(buffer)) {
            outputSize=LENGTHOF(buffer);
            tempbuffer=buffer;
        } else {
            tempbuffer = (UChar *)uprv_malloc(outputSize*U_SIZEOF_UCHAR);

            
            if(tempbuffer == NULL) {
                *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
                if (tempsource != NULL) uprv_free(tempsource);
                return 0;
            }
        }
        uprv_memcpy(tempbuffer, source, sourceLength*U_SIZEOF_UCHAR);
        if (tempsource != NULL){
            uprv_free(tempsource);
        }

        if(sourceLength<outputSize) {
            uprv_memset(tempbuffer+sourceLength, 0, (outputSize-sourceLength)*U_SIZEOF_UCHAR);
        }

        if((options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_LOGICAL) {
            countSpaces(tempbuffer,sourceLength,options,&spacesCountl,&spacesCountr);
            invertBuffer(tempbuffer,sourceLength,options,spacesCountl,spacesCountr);
        }

        if((options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_VISUAL_LTR) {
            if((options&U_SHAPE_SPACES_RELATIVE_TO_TEXT_MASK) == U_SHAPE_SPACES_RELATIVE_TO_TEXT_BEGIN_END) {
                shapeVars.spacesRelativeToTextBeginEnd = 1;
                shapeVars.uShapeLamalefBegin = U_SHAPE_LAMALEF_END;
                shapeVars.uShapeLamalefEnd = U_SHAPE_LAMALEF_BEGIN;
                shapeVars.uShapeTashkeelBegin = U_SHAPE_TASHKEEL_END;
                shapeVars.uShapeTashkeelEnd = U_SHAPE_TASHKEEL_BEGIN;
            }
        }

        switch(options&U_SHAPE_LETTERS_MASK) {
        case U_SHAPE_LETTERS_SHAPE :
             if( (options&U_SHAPE_TASHKEEL_MASK)> 0
                 && ((options&U_SHAPE_TASHKEEL_MASK) !=U_SHAPE_TASHKEEL_REPLACE_BY_TATWEEL)) {
                
                destLength = shapeUnicode(tempbuffer,sourceLength,destCapacity,options,pErrorCode,2,shapeVars);
             }else {
                
                destLength = shapeUnicode(tempbuffer,sourceLength,destCapacity,options,pErrorCode,1,shapeVars);

                
                if( (options&U_SHAPE_TASHKEEL_MASK) == U_SHAPE_TASHKEEL_REPLACE_BY_TATWEEL){
                  destLength = handleTashkeelWithTatweel(tempbuffer,destLength,destCapacity,options,pErrorCode);
                }
            }
            break;
        case U_SHAPE_LETTERS_SHAPE_TASHKEEL_ISOLATED :
            
            destLength = shapeUnicode(tempbuffer,sourceLength,destCapacity,options,pErrorCode,0,shapeVars);
            break;

        case U_SHAPE_LETTERS_UNSHAPE :
            
            destLength = deShapeUnicode(tempbuffer,sourceLength,destCapacity,options,pErrorCode,shapeVars);
            break;
        default :
            
            destLength = 0;
            break;
        }

        






        if((options&U_SHAPE_TEXT_DIRECTION_MASK) == U_SHAPE_TEXT_DIRECTION_LOGICAL) {
            countSpaces(tempbuffer,destLength,options,&spacesCountl,&spacesCountr);
            invertBuffer(tempbuffer,destLength,options,spacesCountl,spacesCountr);
        }
        uprv_memcpy(dest, tempbuffer, uprv_min(destLength, destCapacity)*U_SIZEOF_UCHAR);

        if(tempbuffer!=buffer) {
            uprv_free(tempbuffer);
        }

        if(destLength>destCapacity) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return destLength;
        }

        
    } else {
        



        if(destCapacity<sourceLength) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return sourceLength;
        }
        uprv_memcpy(dest, source, sourceLength*U_SIZEOF_UCHAR);
        destLength=sourceLength;
    }

    





    if((options&U_SHAPE_DIGITS_MASK)!=U_SHAPE_DIGITS_NOOP) {
        UChar digitBase;
        int32_t i;

        
        switch(options&U_SHAPE_DIGIT_TYPE_MASK) {
        case U_SHAPE_DIGIT_TYPE_AN:
            digitBase=0x660; 
            break;
        case U_SHAPE_DIGIT_TYPE_AN_EXTENDED:
            digitBase=0x6f0; 
            break;
        default:
            
            digitBase=0;
            break;
        }

        
        switch(options&U_SHAPE_DIGITS_MASK) {
        case U_SHAPE_DIGITS_EN2AN:
            
            digitBase-=0x30;
            for(i=0; i<destLength; ++i) {
                if(((uint32_t)dest[i]-0x30)<10) {
                    dest[i]+=digitBase;
                }
            }
            break;
        case U_SHAPE_DIGITS_AN2EN:
            
            for(i=0; i<destLength; ++i) {
                if(((uint32_t)dest[i]-(uint32_t)digitBase)<10) {
                    dest[i]-=digitBase-0x30;
                }
            }
            break;
        case U_SHAPE_DIGITS_ALEN2AN_INIT_LR:
            _shapeToArabicDigitsWithContext(dest, destLength,
                                            digitBase,
                                            (UBool)((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_LOGICAL),
                                            FALSE);
            break;
        case U_SHAPE_DIGITS_ALEN2AN_INIT_AL:
            _shapeToArabicDigitsWithContext(dest, destLength,
                                            digitBase,
                                            (UBool)((options&U_SHAPE_TEXT_DIRECTION_MASK)==U_SHAPE_TEXT_DIRECTION_LOGICAL),
                                            TRUE);
            break;
        default:
            
            break;
        }
    }

    return u_terminateUChars(dest, destCapacity, destLength, pErrorCode);
}
