


























#include "lcms.h"
#include <time.h>
#include <stdarg.h>



LCMSAPI DWORD LCMSEXPORT cmsGetPostScriptCSA(cmsHPROFILE hProfile, int Intent, LPVOID Buffer, DWORD dwBufferLen);
LCMSAPI DWORD LCMSEXPORT cmsGetPostScriptCRD(cmsHPROFILE hProfile, int Intent, LPVOID Buffer, DWORD dwBufferLen);
LCMSAPI DWORD LCMSEXPORT cmsGetPostScriptCRDEx(cmsHPROFILE hProfile, int Intent, DWORD dwFlags, LPVOID Buffer, DWORD dwBufferLen);


#define MAXPSCOLS   60      // Columns on tables









































































































































































































































static icTagSignature Device2PCSTab[] = {icSigAToB0Tag,       
                                         icSigAToB1Tag,       
                                         icSigAToB2Tag,       
                                         icSigAToB1Tag };     
                                                           







typedef struct {
                LPBYTE Block;
                LPBYTE Ptr;
                DWORD  dwMax;
                DWORD  dwUsed;          
                int    MaxCols;
                int    Col;
                int    HasError;
                
            } MEMSTREAM, FAR* LPMEMSTREAM;


typedef struct {
                LPLUT Lut;
                LPMEMSTREAM m;
                
                int FirstComponent;
                int SecondComponent;
                
                int   bps;
                const char* PreMaj;
                const char* PostMaj;
                const char* PreMin;
                const char* PostMin;

                int  lIsInput;    
                int  FixWhite;    

                icColorSpaceSignature  ColorSpace;  

                
            } SAMPLERCARGO, FAR* LPSAMPLERCARGO;



static
LPMEMSTREAM CreateMemStream(LPBYTE Buffer, DWORD dwMax, int MaxCols)
{
    LPMEMSTREAM m = (LPMEMSTREAM) _cmsMalloc(sizeof(MEMSTREAM));
    if (m == NULL) return NULL;

    ZeroMemory(m, sizeof(MEMSTREAM));

    m -> Block   = m -> Ptr = Buffer;
    m -> dwMax   = dwMax;
    m -> dwUsed  = 0;
    m -> MaxCols = MaxCols;
    m -> Col     = 0;
    m -> HasError = 0;

    return m;
}




static
BYTE Word2Byte(WORD w)
{
    return (BYTE) floor((double) w / 257.0 + 0.5);
}




static
BYTE L2Byte(WORD w)
{    
	int ww = w + 0x0080;

	if (ww > 0xFFFF) return 0xFF;

    return (BYTE) ((WORD) (ww >> 8) & 0xFF);
}


static
void WriteRawByte(LPMEMSTREAM m, BYTE b)
{    
    if (m -> dwUsed + 1 > m -> dwMax) {
        m -> HasError = 1;      
    }

    if (!m ->HasError && m ->Block) {
        *m ->Ptr++ = b;     
    }
        
    m -> dwUsed++;
}


static
void WriteByte(LPMEMSTREAM m, BYTE b)
{
    static const BYTE Hex[] = "0123456789ABCDEF";
    BYTE c;

        c = Hex[(b >> 4) & 0x0f];
        WriteRawByte(m, c);
    
        c = Hex[b & 0x0f];
        WriteRawByte(m, c);

        m -> Col += 2;

        if (m -> Col > m -> MaxCols) {

            WriteRawByte(m, '\n');
            m -> Col = 0;           
        }       

}


static
void Writef(LPMEMSTREAM m, const char *frm, ...)
{
        va_list args;
        LPBYTE pt;
        BYTE Buffer[2048];

        va_start(args, frm);

        vsnprintf((char*) Buffer, 2048, frm, args);

        for (pt = Buffer; *pt; pt++)  {

            WriteRawByte(m, *pt);               
        }

        va_end(args);       
}







static 
char* RemoveCR(const char* txt)
{
    static char Buffer[2048];
    char* pt;

    strncpy(Buffer, txt, 2047);
    Buffer[2047] = 0;
    for (pt = Buffer; *pt; pt++)
            if (*pt == '\n' || *pt == '\r') *pt = ' ';

    return Buffer;

}

static
void EmitHeader(LPMEMSTREAM m, const char* Title, cmsHPROFILE hProfile)
{

    time_t timer;
    
    time(&timer);

    Writef(m, "%%!PS-Adobe-3.0\n");
    Writef(m, "%%\n");
    Writef(m, "%% %s\n", Title);
    Writef(m, "%% Source: %s\n", RemoveCR(cmsTakeProductName(hProfile)));
    Writef(m, "%% Description: %s\n", RemoveCR(cmsTakeProductDesc(hProfile)));
    Writef(m, "%% Created: %s", ctime(&timer)); 
    Writef(m, "%%\n");
    Writef(m, "%%%%BeginResource\n");

}





static
void EmitWhiteBlackD50(LPMEMSTREAM m, LPcmsCIEXYZ BlackPoint)
{

    Writef(m, "/BlackPoint [%f %f %f]\n", BlackPoint -> X,
                                          BlackPoint -> Y,
                                          BlackPoint -> Z);

    Writef(m, "/WhitePoint [%f %f %f]\n", cmsD50_XYZ()->X, 
                                          cmsD50_XYZ()->Y,
                                          cmsD50_XYZ()->Z);
}


static
void EmitRangeCheck(LPMEMSTREAM m)
{
    Writef(m, "dup 0.0 lt { pop 0.0 } if "
              "dup 1.0 gt { pop 1.0 } if ");

}



static
void EmitIntent(LPMEMSTREAM m, int RenderingIntent)
{
    const char *intent;

    switch (RenderingIntent) {

        case INTENT_PERCEPTUAL:            intent = "Perceptual"; break;
        case INTENT_RELATIVE_COLORIMETRIC: intent = "RelativeColorimetric"; break;
        case INTENT_ABSOLUTE_COLORIMETRIC: intent = "AbsoluteColorimetric"; break;
        case INTENT_SATURATION:            intent = "Saturation"; break;

        default: intent = "Undefined"; break;
    }

    Writef(m, "/RenderingIntent (%s)\n", intent );    
}

























static
void EmitLab2XYZ(LPMEMSTREAM m)
{
    Writef(m, "/RangeABC [ 0 1 0 1 0 1]\n");
    Writef(m, "/DecodeABC [\n");
    Writef(m, "{100 mul  16 add 116 div } bind\n");
    Writef(m, "{255 mul 128 sub 500 div } bind\n");
    Writef(m, "{255 mul 128 sub 200 div } bind\n");
    Writef(m, "]\n");
    Writef(m, "/MatrixABC [ 1 1 1 1 0 0 0 0 -1]\n");
	Writef(m, "/RangeLMN [ -0.236 1.254 0 1 -0.635 1.640 ]\n"); 
    Writef(m, "/DecodeLMN [\n");
    Writef(m, "{dup 6 29 div ge {dup dup mul mul} {4 29 div sub 108 841 div mul} ifelse 0.964200 mul} bind\n");
    Writef(m, "{dup 6 29 div ge {dup dup mul mul} {4 29 div sub 108 841 div mul} ifelse } bind\n");
    Writef(m, "{dup 6 29 div ge {dup dup mul mul} {4 29 div sub 108 841 div mul} ifelse 0.824900 mul} bind\n");
    Writef(m, "]\n");
}





static
void Emit1Gamma(LPMEMSTREAM m, LPWORD Table, int nEntries)
{
    int i;
    double gamma;


    if (nEntries <= 0) return;  

    
    if (cmsIsLinear(Table, nEntries)) return;

    
     gamma = cmsEstimateGammaEx(Table, nEntries, 0.001);
     if (gamma > 0) {
            Writef(m, "{ %g exp } bind ", gamma);
            return;
     }

    Writef(m, "{ ");

    
    EmitRangeCheck(m);
    
    

    
    
                                            
    Writef(m, " [");

    

    for (i=0; i < nEntries; i++) {
            Writef(m, "%d ", Table[i]);
    }

    Writef(m, "] ");                        

    Writef(m, "dup ");                      
    Writef(m, "length 1 sub ");             
    Writef(m, "3 -1 roll ");                
    Writef(m, "mul ");                      
    Writef(m, "dup ");                      
    Writef(m, "dup ");                      
    Writef(m, "floor cvi ");                
    Writef(m, "exch ");                     
    Writef(m, "ceiling cvi ");              
    Writef(m, "3 index ");                  
    Writef(m, "exch ");                     
    Writef(m, "get ");                      
    Writef(m, "4 -1 roll ");                
    Writef(m, "3 -1 roll ");                
    Writef(m, "get ");                      
    Writef(m, "dup ");                      
    Writef(m, "3 1 roll ");                 
    Writef(m, "sub ");                      
    Writef(m, "3 -1 roll ");                
    Writef(m, "dup ");                      
    Writef(m, "floor cvi ");                
    Writef(m, "sub ");                      
    Writef(m, "mul ");                      
    Writef(m, "add ");                      
    Writef(m, "65535 div ");                

    Writef(m, " } bind ");
}




static
LCMSBOOL GammaTableEquals(LPWORD g1, LPWORD g2, int nEntries)
{    
    return memcmp(g1, g2, nEntries* sizeof(WORD)) == 0;
}




static
void EmitNGamma(LPMEMSTREAM m, int n, LPWORD g[], int nEntries)                  
{
    int i;
    
    for( i=0; i < n; i++ )
    {                
        if (i > 0 && GammaTableEquals(g[i-1], g[i], nEntries)) {

            Writef(m, "dup ");
        }
        else {    
            Emit1Gamma(m, g[i], nEntries);
        }
    }
    
}




static
LCMSBOOL IsLUTbased(cmsHPROFILE hProfile, int Intent)
{
    icTagSignature Tag;

    
    Tag = Device2PCSTab[Intent];
        
    if (cmsIsTag(hProfile, Tag)) return 1;

    
    Tag = icSigAToB0Tag;
    
    
    return cmsIsTag(hProfile, Tag);
}




        













static
int OutputValueSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
    LPSAMPLERCARGO sc = (LPSAMPLERCARGO) Cargo;
    unsigned int i;


    if (sc -> FixWhite) {

        if (In[0] == 0xFFFF) {  

            if ((In[1] >= 0x7800 && In[1] <= 0x8800) &&
                (In[2] >= 0x7800 && In[2] <= 0x8800)) {

                WORD* Black;
                WORD* White;
                int nOutputs;

                if (!_cmsEndPointsBySpace(sc ->ColorSpace, &White, &Black, &nOutputs))
                        return 0;

                for (i=0; i < (unsigned int) nOutputs; i++)
                        Out[i] = White[i];
            }

             
        }
    }


    

    if (In[0] != sc ->FirstComponent) {
            
            if (sc ->FirstComponent != -1) {

                    Writef(sc ->m, sc ->PostMin);
                    sc ->SecondComponent = -1;
                    Writef(sc ->m, sc ->PostMaj);           
            }

            
            sc->m->Col = 0;
                    
            Writef(sc ->m, sc ->PreMaj);            
            sc ->FirstComponent = In[0]; 
    }


      if (In[1] != sc ->SecondComponent) {
            
            if (sc ->SecondComponent != -1) {

                    Writef(sc ->m, sc ->PostMin);           
            }
                    
            Writef(sc ->m, sc ->PreMin);            
            sc ->SecondComponent = In[1]; 
    }


    
    
    
    

    for (i=0; i < sc -> Lut ->OutputChan; i++) {

        WORD wWordOut = Out[i];

        if (sc ->bps == 8) {

            
            BYTE wByteOut;
            
            

            if (sc ->lIsInput) {

          
                wByteOut = L2Byte(wWordOut);
            }
            else
                wByteOut = Word2Byte(wWordOut);

            WriteByte(sc -> m, wByteOut);
        }
        else {

            
            WriteByte(sc -> m, (BYTE) (wWordOut & 0xFF));
            WriteByte(sc -> m, (BYTE) ((wWordOut >> 8) & 0xFF));
        }
     }

    return 1;
}



static
void WriteCLUT(LPMEMSTREAM m, LPLUT Lut, int bps, const char* PreMaj, 
                                                  const char* PostMaj,
                                                  const char* PreMin,
                                                  const char* PostMin,
                                                  int lIsInput,
                                                  int FixWhite,
                                                  icColorSpaceSignature ColorSpace)
{
    unsigned int i;
    SAMPLERCARGO sc;

    sc.FirstComponent = -1;
    sc.SecondComponent = -1;
    sc.Lut = Lut;
    sc.m   = m;
    sc.bps = bps;
    sc.PreMaj = PreMaj;
    sc.PostMaj= PostMaj;

    sc.PreMin   = PreMin;
    sc.PostMin  = PostMin;
    sc.lIsInput = lIsInput;
    sc.FixWhite = FixWhite;
    sc.ColorSpace = ColorSpace;

    Writef(m, "[");

    for (i=0; i < Lut ->InputChan; i++)
            Writef(m, " %d ", Lut ->cLutPoints);

    Writef(m, " [\n");

  

    cmsSample3DGrid(Lut, OutputValueSampler, (LPVOID) &sc, SAMPLER_INSPECT);
    
    
    Writef(m, PostMin);
    Writef(m, PostMaj);
    Writef(m, "] ");

    

}




static
int EmitCIEBasedA(LPMEMSTREAM m, LPWORD Tab, int nEntries, LPcmsCIEXYZ BlackPoint)
{
            
        Writef(m, "[ /CIEBasedA\n");
        Writef(m, "  <<\n");

        Writef(m, "/DecodeA ");

        Emit1Gamma(m,Tab, nEntries);

        Writef(m, " \n");

        Writef(m, "/MatrixA [ 0.9642 1.0000 0.8249 ]\n");
        Writef(m, "/RangeLMN [ 0.0 0.9642 0.0 1.0000 0.0 0.8249 ]\n");
        
        EmitWhiteBlackD50(m, BlackPoint);
        EmitIntent(m, INTENT_PERCEPTUAL);

        Writef(m, ">>\n");        
        Writef(m, "]\n");
        
        return 1;
}




static
int EmitCIEBasedABC(LPMEMSTREAM m, LPWORD L[], int nEntries, LPWMAT3 Matrix, LPcmsCIEXYZ BlackPoint)
{
    int i;

        Writef(m, "[ /CIEBasedABC\n");
        Writef(m, "<<\n");
        Writef(m, "/DecodeABC [ ");
 
        EmitNGamma(m, 3, L, nEntries);

        Writef(m, "]\n");
        
        Writef(m, "/MatrixABC [ " );

        for( i=0; i < 3; i++ ) {
            
            Writef(m, "%.6f %.6f %.6f ", 
                        FIXED_TO_DOUBLE(Matrix->v[0].n[i]),
                        FIXED_TO_DOUBLE(Matrix->v[1].n[i]),
                        FIXED_TO_DOUBLE(Matrix->v[2].n[i]));            
        }

    
        Writef(m, "]\n");

        Writef(m, "/RangeLMN [ 0.0 0.9642 0.0 1.0000 0.0 0.8249 ]\n");
        
        EmitWhiteBlackD50(m, BlackPoint);
        EmitIntent(m, INTENT_PERCEPTUAL);

        Writef(m, ">>\n");
        Writef(m, "]\n");
        

        return 1;
}


static
int EmitCIEBasedDEF(LPMEMSTREAM m, LPLUT Lut, int Intent, LPcmsCIEXYZ BlackPoint)
{
    const char* PreMaj;
    const char* PostMaj;
    const char* PreMin, *PostMin;

    switch (Lut ->InputChan) {
    case 3:

            Writef(m, "[ /CIEBasedDEF\n");
            PreMaj ="<"; 
            PostMaj= ">\n";
            PreMin = PostMin = "";
            break;
    case 4:
            Writef(m, "[ /CIEBasedDEFG\n");
            PreMaj = "[";
            PostMaj = "]\n";
            PreMin = "<";
            PostMin = ">\n";
            break;
    default:
            return 0;

    }

    Writef(m, "<<\n");

    if (Lut ->wFlags & LUT_HASTL1) {
    
        Writef(m, "/DecodeDEF [ ");
        EmitNGamma(m, Lut ->InputChan, Lut ->L1, Lut ->CLut16params.nSamples);
        Writef(m, "]\n");
    }



    if (Lut ->wFlags & LUT_HAS3DGRID) {

            Writef(m, "/Table ");    
            WriteCLUT(m, Lut, 8, PreMaj, PostMaj, PreMin, PostMin, TRUE, FALSE, (icColorSpaceSignature) 0);
            Writef(m, "]\n");
    }
       
    EmitLab2XYZ(m);
    EmitWhiteBlackD50(m, BlackPoint);
    EmitIntent(m, Intent);

    Writef(m, "   >>\n");       
    Writef(m, "]\n");
    

    return 1;
}



static
LPGAMMATABLE ExtractGray2Y(cmsHPROFILE hProfile, int Intent)
{
    LPGAMMATABLE Out = cmsAllocGamma(256);
    cmsHPROFILE hXYZ = cmsCreateXYZProfile();
    cmsHTRANSFORM xform = cmsCreateTransform(hProfile, TYPE_GRAY_8, hXYZ, TYPE_XYZ_DBL, Intent, cmsFLAGS_NOTPRECALC);
    int i;

    for (i=0; i < 256; i++) {
        
      BYTE Gray = (BYTE) i;
      cmsCIEXYZ XYZ;
      
        cmsDoTransform(xform, &Gray, &XYZ, 1);
        
        Out ->GammaTable[i] =_cmsClampWord((int) floor(XYZ.Y * 65535.0 + 0.5));
    }

    cmsDeleteTransform(xform);
    cmsCloseProfile(hXYZ);
    return Out;
}






static
int WriteInputLUT(LPMEMSTREAM m, cmsHPROFILE hProfile, int Intent)
{
    cmsHPROFILE hLab;
    cmsHTRANSFORM xform;
    icColorSpaceSignature ColorSpace;
    int nChannels;
    DWORD InputFormat;
    int rc;
    cmsHPROFILE Profiles[2];
    cmsCIEXYZ BlackPointAdaptedToD50;

    
    
    
    hLab        = cmsCreateLabProfile(NULL);
    ColorSpace  =  cmsGetColorSpace(hProfile);
    nChannels   = _cmsChannelsOf(ColorSpace);
    InputFormat = CHANNELS_SH(nChannels) | BYTES_SH(2);

    cmsDetectBlackPoint(&BlackPointAdaptedToD50, hProfile, Intent,LCMS_BPFLAGS_D50_ADAPTED);

    
    if (cmsGetDeviceClass(hProfile) == icSigLinkClass) {

        

        if (cmsGetPCS(hProfile) == icSigLabData) {
            
            xform = cmsCreateTransform(hProfile, InputFormat, NULL, 
                            TYPE_Lab_DBL, Intent, 0);
        }
        else {

            

            Profiles[0] = hProfile;
            Profiles[1] = hLab;

            xform = cmsCreateMultiprofileTransform(Profiles, 2,  InputFormat, 
                                    TYPE_Lab_DBL, Intent, 0);
        }


    }
    else {

        
        xform = cmsCreateTransform(hProfile, InputFormat, hLab, 
                            TYPE_Lab_DBL, Intent, 0);
    }


    
    if (xform == NULL) {
                        
            cmsSignalError(LCMS_ERRC_ABORTED, "Cannot create transform Profile -> Lab");
            return 0;
    }

    

    switch (nChannels) {

    case 1: {            
            LPGAMMATABLE Gray2Y = ExtractGray2Y(hProfile, Intent);
            EmitCIEBasedA(m, Gray2Y->GammaTable, Gray2Y ->nEntries, &BlackPointAdaptedToD50);            
            cmsFreeGamma(Gray2Y);            
            }
            break;

    case 3: 
    case 4: {
            LPLUT DeviceLink;
            _LPcmsTRANSFORM v = (_LPcmsTRANSFORM) xform;

            if (v ->DeviceLink) 
                rc = EmitCIEBasedDEF(m, v->DeviceLink, Intent, &BlackPointAdaptedToD50);
            else {
                DeviceLink = _cmsPrecalculateDeviceLink(xform, 0);
                rc = EmitCIEBasedDEF(m, DeviceLink, Intent, &BlackPointAdaptedToD50);
                cmsFreeLUT(DeviceLink);
            }
            }
            break;

    default:

            cmsSignalError(LCMS_ERRC_ABORTED, "Only 3, 4 channels supported for CSA. This profile has %d channels.", nChannels);
            return 0;
    }
    

    cmsDeleteTransform(xform);
    cmsCloseProfile(hLab);
    return 1;
}





static
int WriteInputMatrixShaper(LPMEMSTREAM m, cmsHPROFILE hProfile)
{
    icColorSpaceSignature ColorSpace;
    LPMATSHAPER MatShaper;
    int rc;
    cmsCIEXYZ BlackPointAdaptedToD50;


    ColorSpace = cmsGetColorSpace(hProfile);
    MatShaper  = cmsBuildInputMatrixShaper(hProfile);

    cmsDetectBlackPoint(&BlackPointAdaptedToD50, hProfile, INTENT_RELATIVE_COLORIMETRIC, LCMS_BPFLAGS_D50_ADAPTED);

    if (MatShaper == NULL) {

                cmsSignalError(LCMS_ERRC_ABORTED, "This profile is not suitable for input");
                return 0;
    }

    if (ColorSpace == icSigGrayData) {
            
            rc = EmitCIEBasedA(m, MatShaper ->L[0], 
                                  MatShaper ->p16.nSamples,
                                  &BlackPointAdaptedToD50);
        
    }
    else
        if (ColorSpace == icSigRgbData) {

        
            rc = EmitCIEBasedABC(m, MatShaper->L, 
                                        MatShaper ->p16.nSamples, 
                                        &MatShaper ->Matrix,
                                        &BlackPointAdaptedToD50);      
        }
        else  {

            cmsSignalError(LCMS_ERRC_ABORTED, "Profile is not suitable for CSA. Unsupported colorspace.");
            return 0;
        }

    cmsFreeMatShaper(MatShaper);
    return rc;
}






static
int WriteNamedColorCSA(LPMEMSTREAM m, cmsHPROFILE hNamedColor, int Intent)
{
    cmsHTRANSFORM xform;
    cmsHPROFILE   hLab;
    int i, nColors;
    char ColorName[32];


    hLab  = cmsCreateLabProfile(NULL);
    xform = cmsCreateTransform(hNamedColor, TYPE_NAMED_COLOR_INDEX, 
                        hLab, TYPE_Lab_DBL, Intent, cmsFLAGS_NOTPRECALC);
    if (xform == NULL) return 0;


    Writef(m, "<<\n");
    Writef(m, "(colorlistcomment) (%s)\n", "Named color CSA");
    Writef(m, "(Prefix) [ (Pantone ) (PANTONE ) ]\n");
    Writef(m, "(Suffix) [ ( CV) ( CVC) ( C) ]\n");

    nColors   = cmsNamedColorCount(xform);


    for (i=0; i < nColors; i++) {
        
        WORD In[1];
        cmsCIELab Lab;

        In[0] = (WORD) i;

        if (!cmsNamedColorInfo(xform, i, ColorName, NULL, NULL))
                continue;

        cmsDoTransform(xform, In, &Lab, 1);     
        Writef(m, "  (%s) [ %.3f %.3f %.3f ]\n", ColorName, Lab.L, Lab.a, Lab.b);
    }


        
    Writef(m, ">>\n");

    cmsDeleteTransform(xform);
    cmsCloseProfile(hLab);
    return 1;
}




DWORD LCMSEXPORT cmsGetPostScriptCSA(cmsHPROFILE hProfile, 
                              int Intent, 
                              LPVOID Buffer, DWORD dwBufferLen)
{
    
    LPMEMSTREAM mem;
    DWORD dwBytesUsed;
    
    
    mem = CreateMemStream((LPBYTE) Buffer, dwBufferLen, MAXPSCOLS);
    if (!mem) return 0;

    
    
    if (cmsGetDeviceClass(hProfile) == icSigNamedColorClass) {

        if (!WriteNamedColorCSA(mem, hProfile, Intent)) {

                    _cmsFree((void*) mem);
                    return 0;
        }
    }
    else {


    
    
    icColorSpaceSignature ColorSpace = cmsGetPCS(hProfile);

    if (ColorSpace != icSigXYZData &&
        ColorSpace != icSigLabData) {

            cmsSignalError(LCMS_ERRC_ABORTED, "Invalid output color space");
            _cmsFree((void*) mem);
            return 0;
    }
    
    
    if (IsLUTbased(hProfile, Intent)) {

        
        if (!WriteInputLUT(mem, hProfile, Intent)) {

                    _cmsFree((void*) mem);
                    return 0;
        }
    }
    else {
        
        

        if (!WriteInputMatrixShaper(mem, hProfile)) {

                    _cmsFree((void*) mem);  
                    return 0;
        }
    }
    }

    
    
    dwBytesUsed = mem ->dwUsed;

    
    _cmsFree((void*) mem);

    
    return dwBytesUsed;
}



































































static
void EmitPQRStage(LPMEMSTREAM m, cmsHPROFILE hProfile, int DoBPC, int lIsAbsolute)
{

   
        if (lIsAbsolute) {

            
			

			

			cmsCIEXYZ White;

			cmsTakeMediaWhitePoint(&White, hProfile);

			Writef(m,"/MatrixPQR [1 0 0 0 1 0 0 0 1 ]\n");
            Writef(m,"/RangePQR [ -0.5 2 -0.5 2 -0.5 2 ]\n");

            Writef(m, "%% Absolute colorimetric -- encode to relative to maximize LUT usage\n"
                      "/TransformPQR [\n"
                      "{0.9642 mul %g div exch pop exch pop exch pop exch pop} bind\n"
                      "{1.0000 mul %g div exch pop exch pop exch pop exch pop} bind\n"
                      "{0.8249 mul %g div exch pop exch pop exch pop exch pop} bind\n]\n", 
					  White.X, White.Y, White.Z);
            return;
        }


        Writef(m,"%% Bradford Cone Space\n"
                 "/MatrixPQR [0.8951 -0.7502 0.0389 0.2664 1.7135 -0.0685 -0.1614 0.0367 1.0296 ] \n");

        Writef(m, "/RangePQR [ -0.5 2 -0.5 2 -0.5 2 ]\n");


        

        if (!DoBPC) {

            Writef(m, "%% VonKries-like transform in Bradford Cone Space\n"
                      "/TransformPQR [\n"
                      "{exch pop exch 3 get mul exch pop exch 3 get div} bind\n"
                      "{exch pop exch 4 get mul exch pop exch 4 get div} bind\n"
                      "{exch pop exch 5 get mul exch pop exch 5 get div} bind\n]\n"); 
        } else {

            

            Writef(m, "%% VonKries-like transform in Bradford Cone Space plus BPC\n"
                      "/TransformPQR [\n");
                  
            Writef(m, "{4 index 3 get div 2 index 3 get mul "
                    "2 index 3 get 2 index 3 get sub mul "                          
                    "2 index 3 get 4 index 3 get 3 index 3 get sub mul sub "
                    "3 index 3 get 3 index 3 get exch sub div "
                    "exch pop exch pop exch pop exch pop } bind\n");

            Writef(m, "{4 index 4 get div 2 index 4 get mul "
                    "2 index 4 get 2 index 4 get sub mul "
                    "2 index 4 get 4 index 4 get 3 index 4 get sub mul sub "
                    "3 index 4 get 3 index 4 get exch sub div "
                    "exch pop exch pop exch pop exch pop } bind\n");

            Writef(m, "{4 index 5 get div 2 index 5 get mul "
                    "2 index 5 get 2 index 5 get sub mul "
                    "2 index 5 get 4 index 5 get 3 index 5 get sub mul sub "
                    "3 index 5 get 3 index 5 get exch sub div "
                    "exch pop exch pop exch pop exch pop } bind\n]\n");

        }
          
        
}


static
void EmitXYZ2Lab(LPMEMSTREAM m)
{
    Writef(m, "/RangeLMN [ -0.635 2.0 0 2 -0.635 2.0 ]\n"); 
    Writef(m, "/EncodeLMN [\n");
    Writef(m, "{ 0.964200  div dup 0.008856 le {7.787 mul 16 116 div add}{1 3 div exp} ifelse } bind\n");
    Writef(m, "{ 1.000000  div dup 0.008856 le {7.787 mul 16 116 div add}{1 3 div exp} ifelse } bind\n");
    Writef(m, "{ 0.824900  div dup 0.008856 le {7.787 mul 16 116 div add}{1 3 div exp} ifelse } bind\n");
    Writef(m, "]\n");
    Writef(m, "/MatrixABC [ 0 1 0 1 -1 1 0 0 -1 ]\n");
    Writef(m, "/EncodeABC [\n");
    
        
    Writef(m, "{ 116 mul  16 sub 100 div  } bind\n");
    Writef(m, "{ 500 mul 128 add 256 div  } bind\n");
    Writef(m, "{ 200 mul 128 add 256 div  } bind\n");
    
    
    Writef(m, "]\n");
    

}







static
int WriteOutputLUT(LPMEMSTREAM m, cmsHPROFILE hProfile, int Intent, DWORD dwFlags)
{
    cmsHPROFILE hLab;
    cmsHTRANSFORM xform;
    icColorSpaceSignature ColorSpace;
    int i, nChannels;
    DWORD OutputFormat;
    _LPcmsTRANSFORM v;
    LPLUT DeviceLink;
    cmsHPROFILE Profiles[3];
    cmsCIEXYZ BlackPointAdaptedToD50;
    LCMSBOOL lFreeDeviceLink = FALSE;
    LCMSBOOL lDoBPC = (dwFlags & cmsFLAGS_BLACKPOINTCOMPENSATION);
    LCMSBOOL lFixWhite = !(dwFlags & cmsFLAGS_NOWHITEONWHITEFIXUP);
	int RelativeEncodingIntent;
    
    

    hLab = cmsCreateLabProfile(NULL);

    ColorSpace  =  cmsGetColorSpace(hProfile);
    nChannels   = _cmsChannelsOf(ColorSpace);
    OutputFormat = CHANNELS_SH(nChannels) | BYTES_SH(2);
    
	
	

    RelativeEncodingIntent = Intent;
	if (RelativeEncodingIntent == INTENT_ABSOLUTE_COLORIMETRIC)
		RelativeEncodingIntent = INTENT_RELATIVE_COLORIMETRIC;


    
    if (cmsGetDeviceClass(hProfile) == icSigLinkClass) {

        

        if (ColorSpace == icSigLabData) {

              

            Profiles[0] = hLab;
            Profiles[1] = hProfile;

            xform = cmsCreateMultiprofileTransform(Profiles, 2, TYPE_Lab_DBL, 
                                                        OutputFormat, RelativeEncodingIntent, 
														dwFlags|cmsFLAGS_NOWHITEONWHITEFIXUP|cmsFLAGS_NOPRELINEARIZATION);
            
        }
        else {
          cmsSignalError(LCMS_ERRC_ABORTED, "Cannot use devicelink profile for CRD creation");
          return 0;
        }


    }
    else {
		
        
        xform = cmsCreateTransform(hLab, TYPE_Lab_DBL, hProfile, 
                            OutputFormat, RelativeEncodingIntent, dwFlags|cmsFLAGS_NOWHITEONWHITEFIXUP|cmsFLAGS_NOPRELINEARIZATION);
    }

    if (xform == NULL) {
                        
            cmsSignalError(LCMS_ERRC_ABORTED, "Cannot create transform Lab -> Profile in CRD creation");
            return 0;
    }

    

    v = (_LPcmsTRANSFORM) xform;
    DeviceLink = v ->DeviceLink;
    
    if (!DeviceLink) {

        DeviceLink = _cmsPrecalculateDeviceLink(xform, cmsFLAGS_NOPRELINEARIZATION);
        lFreeDeviceLink = TRUE;
    }

    Writef(m, "<<\n");
    Writef(m, "/ColorRenderingType 1\n");


    cmsDetectBlackPoint(&BlackPointAdaptedToD50, hProfile, Intent, LCMS_BPFLAGS_D50_ADAPTED);

    
    EmitWhiteBlackD50(m, &BlackPointAdaptedToD50);
    EmitPQRStage(m, hProfile, lDoBPC, Intent == INTENT_ABSOLUTE_COLORIMETRIC);
    EmitXYZ2Lab(m);
        
    if (DeviceLink ->wFlags & LUT_HASTL1) {

        
        cmsSignalError(LCMS_ERRC_ABORTED, "Internal error (prelinearization on CRD)");
        return 0;
    }
    

    
    
    
    
    
    
    if (Intent == INTENT_ABSOLUTE_COLORIMETRIC)
            lFixWhite = FALSE;

    Writef(m, "/RenderTable ");
    
    WriteCLUT(m, DeviceLink, 8, "<", ">\n", "", "", FALSE, 
                lFixWhite, ColorSpace);
    
    Writef(m, " %d {} bind ", nChannels);

    for (i=1; i < nChannels; i++)
            Writef(m, "dup ");

    Writef(m, "]\n");

        
    EmitIntent(m, Intent);

    Writef(m, ">>\n");

    if (!(dwFlags & cmsFLAGS_NODEFAULTRESOURCEDEF)) {

        Writef(m, "/Current exch /ColorRendering defineresource pop\n");
    }

    if (lFreeDeviceLink) cmsFreeLUT(DeviceLink);
    cmsDeleteTransform(xform);
    cmsCloseProfile(hLab);

    return 1;   
}



static
void BuildColorantList(char *Colorant, int nColorant, WORD Out[])
{
    char Buff[32];
    int j;

    Colorant[0] = 0;
    if (nColorant > MAXCHANNELS)
        nColorant = MAXCHANNELS;

    for (j=0; j < nColorant; j++) {

                sprintf(Buff, "%.3f", Out[j] / 65535.0);
                strcat(Colorant, Buff);
                if (j < nColorant -1) 
                        strcat(Colorant, " ");

        }       
}





static
int WriteNamedColorCRD(LPMEMSTREAM m, cmsHPROFILE hNamedColor, int Intent, DWORD dwFlags)
{
    cmsHTRANSFORM xform;    
    int i, nColors, nColorant;
    DWORD OutputFormat;
    char ColorName[32];
    char Colorant[128];

    nColorant = _cmsChannelsOf(cmsGetColorSpace(hNamedColor));
    OutputFormat = CHANNELS_SH(nColorant) | BYTES_SH(2);

    xform = cmsCreateTransform(hNamedColor, TYPE_NAMED_COLOR_INDEX, 
                        NULL, OutputFormat, Intent, cmsFLAGS_NOTPRECALC);
    if (xform == NULL) return 0;


    Writef(m, "<<\n");
    Writef(m, "(colorlistcomment) (%s) \n", "Named profile");
    Writef(m, "(Prefix) [ (Pantone ) (PANTONE ) ]\n");
    Writef(m, "(Suffix) [ ( CV) ( CVC) ( C) ]\n");

    nColors   = cmsNamedColorCount(xform);
    

    for (i=0; i < nColors; i++) {
        
        WORD In[1];
        WORD Out[MAXCHANNELS];

        In[0] = (WORD) i;

        if (!cmsNamedColorInfo(xform, i, ColorName, NULL, NULL))
                continue;

        cmsDoTransform(xform, In, Out, 1);      
        BuildColorantList(Colorant, nColorant, Out);
        Writef(m, "  (%s) [ %s ]\n", ColorName, Colorant);
    }

    Writef(m, "   >>");

    if (!(dwFlags & cmsFLAGS_NODEFAULTRESOURCEDEF)) {

    Writef(m, " /Current exch /HPSpotTable defineresource pop\n");
    }

    cmsDeleteTransform(xform);  
    return 1;
}







DWORD LCMSEXPORT cmsGetPostScriptCRDEx(cmsHPROFILE hProfile, 
                              int Intent, DWORD dwFlags,
                              LPVOID Buffer, DWORD dwBufferLen)
{
    
    LPMEMSTREAM mem;
    DWORD dwBytesUsed;

    
    mem = CreateMemStream((LPBYTE) Buffer, dwBufferLen, MAXPSCOLS);
    if (!mem) return 0;

    
    if (!(dwFlags & cmsFLAGS_NODEFAULTRESOURCEDEF)) {

    EmitHeader(mem, "Color Rendering Dictionary (CRD)", hProfile);
    }


    
    if (cmsGetDeviceClass(hProfile) == icSigNamedColorClass) {

        if (!WriteNamedColorCRD(mem, hProfile, Intent, dwFlags)) {

                    _cmsFree((void*) mem);
                    return 0;
        }
    }
    else {
        
    


    if (!WriteOutputLUT(mem, hProfile, Intent, dwFlags)) {
        _cmsFree((void*) mem);
        return 0;
    }
    }
    
    if (!(dwFlags & cmsFLAGS_NODEFAULTRESOURCEDEF)) {

    Writef(mem, "%%%%EndResource\n");
    Writef(mem, "\n%% CRD End\n");
    }

    
    dwBytesUsed = mem ->dwUsed;

    
    _cmsFree((void*) mem);

    
    return dwBytesUsed;
}




DWORD LCMSEXPORT cmsGetPostScriptCRD(cmsHPROFILE hProfile, 
                              int Intent, 
                              LPVOID Buffer, DWORD dwBufferLen)
{
    return cmsGetPostScriptCRDEx(hProfile, Intent, 0, Buffer, dwBufferLen);
}
