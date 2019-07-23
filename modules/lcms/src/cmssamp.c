






















#include "lcms.h"




static volatile int GlobalBlackPreservationStrategy = 0;



WORD _cmsQuantizeVal(double i, int MaxSamples)
{
       double x;

       x = ((double) i * 65535.) / (double) (MaxSamples - 1);

       return (WORD) floor(x + .5);
}




int cmsIsLinear(WORD Table[], int nEntries)
{
       register int i;
       int diff;

       for (i=0; i < nEntries; i++) {

           diff = abs((int) Table[i] - (int) _cmsQuantizeVal(i, nEntries));              
           if (diff > 3)
                     return 0;
       }

       return 1;
}





static
int ipow(int base, int exp)
{
        int res = base;

        while (--exp)
               res *= base;

        return res;
}




static
int ComponentOf(int n, int clut, int nColorant)
{
        if (nColorant <= 0)
                return (n % clut);

        n /= ipow(clut, nColorant);

        return (n % clut);
}






LCMSBOOL LCMSEXPORT cmsSample3DGrid(LPLUT Lut, _cmsSAMPLER Sampler, LPVOID Cargo, DWORD dwFlags)
{
   int i, t, nTotalPoints, Colorant, index;
   WORD In[MAXCHANNELS], Out[MAXCHANNELS];

   nTotalPoints = ipow(Lut->cLutPoints, Lut -> InputChan);

   index = 0;
   for (i = 0; i < nTotalPoints; i++) {

        for (t=0; t < (int) Lut -> InputChan; t++) {

                Colorant =  ComponentOf(i, Lut -> cLutPoints, (Lut -> InputChan - t  - 1 ));
                In[t]    = _cmsQuantizeVal(Colorant, Lut -> cLutPoints);
        }


        if (dwFlags & SAMPLER_HASTL1) {

                 for (t=0; t < (int) Lut -> InputChan; t++)
                     In[t] = cmsReverseLinearInterpLUT16(In[t],
                                                Lut -> L1[t],
                                                &Lut -> In16params);
        }

		for (t=0; t < (int) Lut -> OutputChan; t++)
                     Out[t] = Lut->T[index + t];

        if (dwFlags & SAMPLER_HASTL2) {

             for (t=0; t < (int) Lut -> OutputChan; t++)
                     Out[t] = cmsLinearInterpLUT16(Out[t],
                                                   Lut -> L2[t],
                                                   &Lut -> Out16params);               
        }	


        if (!Sampler(In, Out, Cargo))
                return FALSE;

        if (!(dwFlags & SAMPLER_INSPECT)) {
            
            if (dwFlags & SAMPLER_HASTL2) {

                for (t=0; t < (int) Lut -> OutputChan; t++)
                     Out[t] = cmsReverseLinearInterpLUT16(Out[t],
                                                   Lut -> L2[t],
                                                   &Lut -> Out16params);
                }

        
            for (t=0; t < (int) Lut -> OutputChan; t++)
                        Lut->T[index + t] = Out[t];

        }

        index += Lut -> OutputChan;

    }

    return TRUE;
}







int _cmsReasonableGridpointsByColorspace(icColorSpaceSignature Colorspace, DWORD dwFlags)
{
    int nChannels;

    
    if (dwFlags & 0x00FF0000) {
            
            return (dwFlags >> 16) & 0xFF;
    }

    nChannels = _cmsChannelsOf(Colorspace);

    

    if (dwFlags & cmsFLAGS_HIGHRESPRECALC) {

        if (nChannels > 4) 
                return 7;       

        if (nChannels == 4)     
                return 23;
    
        return 49;      
    }


    

    if (dwFlags & cmsFLAGS_LOWRESPRECALC) {
        
        if (nChannels > 4) 
                return 6;       

        if (nChannels == 1) 
                return 33;      

        return 17;              
    }

    

    if (nChannels > 4) 
                return 7;       

    if (nChannels == 4)
                return 17;      

    return 33;                  
    
}




static
int XFormSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
        cmsDoTransform((cmsHTRANSFORM) Cargo, In, Out, 1);
        return TRUE;
}




LPLUT _cmsPrecalculateDeviceLink(cmsHTRANSFORM h, DWORD dwFlags)
{
       _LPcmsTRANSFORM p = (_LPcmsTRANSFORM) h;
       LPLUT Grid;
       int nGridPoints;
       DWORD dwFormatIn, dwFormatOut;
       DWORD SaveFormatIn, SaveFormatOut;
       int ChannelsIn, ChannelsOut;
       LPLUT SaveGamutLUT;


       
       SaveGamutLUT = p ->Gamut;
       p ->Gamut = NULL;

       ChannelsIn   = _cmsChannelsOf(p -> EntryColorSpace);
       ChannelsOut  = _cmsChannelsOf(p -> ExitColorSpace);
               
       nGridPoints = _cmsReasonableGridpointsByColorspace(p -> EntryColorSpace, dwFlags);
     
       Grid =  cmsAllocLUT();
       if (!Grid) return NULL;

       Grid = cmsAlloc3DGrid(Grid, nGridPoints, ChannelsIn, ChannelsOut);

       
       dwFormatIn   = (CHANNELS_SH(ChannelsIn)|BYTES_SH(2));
       dwFormatOut  = (CHANNELS_SH(ChannelsOut)|BYTES_SH(2));

       SaveFormatIn  = p ->InputFormat;
       SaveFormatOut = p ->OutputFormat;

       p -> InputFormat  = dwFormatIn;
       p -> OutputFormat = dwFormatOut;
       p -> FromInput    = _cmsIdentifyInputFormat(p, dwFormatIn);
       p -> ToOutput     = _cmsIdentifyOutputFormat(p, dwFormatOut);

       
           
       if (!(dwFlags & cmsFLAGS_NOPRELINEARIZATION)) {

           cmsHTRANSFORM hOne[1];
           hOne[0] = h;
                
           _cmsComputePrelinearizationTablesFromXFORM(hOne, 1, Grid);
       }
                    
       
       
       

       if (!cmsSample3DGrid(Grid, XFormSampler, (LPVOID) p, Grid -> wFlags)) {

                cmsFreeLUT(Grid);
                Grid = NULL;
       }      
      
       p ->Gamut        = SaveGamutLUT;
       p ->InputFormat  = SaveFormatIn; 
       p ->OutputFormat = SaveFormatOut;

       return Grid;
}





typedef struct {
                cmsHTRANSFORM cmyk2cmyk;
                cmsHTRANSFORM cmyk2Lab;
                LPGAMMATABLE  KTone;
                L16PARAMS     KToneParams;
                LPLUT         LabK2cmyk;
                double        MaxError;

                cmsHTRANSFORM hRoundTrip;               
                int           MaxTAC;

                cmsHTRANSFORM hProofOutput;

    } BPCARGO, *LPBPCARGO;




static
int BlackPreservingGrayOnlySampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
    BPCARGO* bp = (LPBPCARGO) Cargo;

    
    if (In[0] == 0 && In[1] == 0 && In[2] == 0) {

        
        Out[0] = Out[1] = Out[2] = 0;
        Out[3] = cmsLinearInterpLUT16(In[3], bp->KTone ->GammaTable, &bp->KToneParams);
        return 1;
    }

    
    cmsDoTransform(bp ->cmyk2cmyk, In, Out, 1);
    return 1;
}




static
int BlackPreservingSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{

    WORD LabK[4];   
    double SumCMY, SumCMYK, Error;
    cmsCIELab ColorimetricLab, BlackPreservingLab;
    BPCARGO* bp = (LPBPCARGO) Cargo;
    
    
    LabK[3] = cmsLinearInterpLUT16(In[3], bp->KTone ->GammaTable, &bp->KToneParams);

    
    if (In[0] == 0 && In[1] == 0 && In[2] == 0) {

        Out[0] = Out[1] = Out[2] = 0;
        Out[3] = LabK[3];
        return 1;
    }
    
    
    cmsDoTransform(bp ->cmyk2cmyk, In, Out, 1);
    if (Out[3] == LabK[3]) return 1;
    

    
    cmsDoTransform(bp->hProofOutput, Out, &ColorimetricLab, 1);
    
    
    
    cmsDoTransform(bp ->cmyk2Lab, In, LabK, 1);
        
    
    
    cmsEvalLUTreverse(bp ->LabK2cmyk, LabK, Out, Out); 
        
    
    cmsDoTransform(bp->hProofOutput, Out, &BlackPreservingLab, 1);  
    Error = cmsDeltaE(&ColorimetricLab, &BlackPreservingLab);

    
    
    
    SumCMY   = Out[0]  + Out[1] + Out[2];
    SumCMYK  = SumCMY + Out[3];      

    if (SumCMYK > bp ->MaxTAC) {

        double Ratio = 1 - ((SumCMYK - bp->MaxTAC) / SumCMY);
        if (Ratio < 0)
                  Ratio = 0;
                
        Out[0] = (WORD) floor(Out[0] * Ratio + 0.5);     
        Out[1] = (WORD) floor(Out[1] * Ratio + 0.5);     
        Out[2] = (WORD) floor(Out[2] * Ratio + 0.5);     
    }
                        
    return 1;
}




#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

static
int EstimateTAC(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
    BPCARGO* bp = (LPBPCARGO) Cargo;
    WORD RoundTrip[4];
    int Sum;

    cmsDoTransform(bp->hRoundTrip, In, RoundTrip, 1);
    
    Sum = RoundTrip[0] + RoundTrip[1] + RoundTrip[2] + RoundTrip[3];

    if (Sum > bp ->MaxTAC)
            bp ->MaxTAC = Sum;
    
    return 1;
}



static
int BlackPreservingEstimateErrorSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
    BPCARGO* bp = (LPBPCARGO) Cargo;
    WORD ColorimetricOut[4];
    cmsCIELab ColorimetricLab, BlackPreservingLab;
    double Error;
        
    if (In[0] == 0 && In[1] == 0 && In[2] == 0) return 1;

    cmsDoTransform(bp->cmyk2cmyk, In, ColorimetricOut, 1);

    cmsDoTransform(bp->hProofOutput, ColorimetricOut, &ColorimetricLab, 1); 
    cmsDoTransform(bp->hProofOutput, Out, &BlackPreservingLab, 1);
        
    Error = cmsDeltaE(&ColorimetricLab, &BlackPreservingLab);

    if (Error > bp ->MaxError)
        bp ->MaxError = Error;

    return 1;
}


int LCMSEXPORT cmsSetCMYKPreservationStrategy(int n)
{
    int OldVal = GlobalBlackPreservationStrategy;

    if (n >= 0) 
            GlobalBlackPreservationStrategy = n;

    return OldVal;
}

#pragma warning(disable: 4550)


static
_cmsSAMPLER _cmsGetBlackPreservationSampler(void)
{
    switch (GlobalBlackPreservationStrategy) {

        case 0: return BlackPreservingGrayOnlySampler;
        default: return BlackPreservingSampler;
   }

}


LPLUT _cmsPrecalculateBlackPreservingDeviceLink(cmsHTRANSFORM hCMYK2CMYK, DWORD dwFlags)
{
       _LPcmsTRANSFORM p = (_LPcmsTRANSFORM) hCMYK2CMYK;
       BPCARGO Cargo;      
       LPLUT Grid;
       DWORD LocalFlags;
       cmsHPROFILE hLab = cmsCreateLabProfile(NULL);
       int nGridPoints;    
       icTagSignature Device2PCS[] = {icSigAToB0Tag,       
                                      icSigAToB1Tag,       
                                      icSigAToB2Tag,       
                                      icSigAToB1Tag };     
                                                           
           
       nGridPoints = _cmsReasonableGridpointsByColorspace(p -> EntryColorSpace, dwFlags);
     
       
       LocalFlags = cmsFLAGS_NOTPRECALC;
       if (p -> dwOriginalFlags & cmsFLAGS_BLACKPOINTCOMPENSATION)
           LocalFlags |= cmsFLAGS_BLACKPOINTCOMPENSATION;

       
       Cargo.cmyk2cmyk = hCMYK2CMYK;

       
       Cargo.KTone  =  _cmsBuildKToneCurve(hCMYK2CMYK, 256);
       if (Cargo.KTone == NULL) return NULL;   		
       cmsCalcL16Params(Cargo.KTone ->nEntries, &Cargo.KToneParams);
       

       
       Cargo.cmyk2Lab  = cmsCreateTransform(p ->InputProfile, TYPE_CMYK_16, 
                                            hLab, TYPE_Lab_16, p->Intent, LocalFlags);

       
       Cargo.LabK2cmyk = cmsReadICCLut(p->OutputProfile, Device2PCS[p->Intent]);

       
	   if (Cargo.LabK2cmyk == NULL) {

		   Grid = NULL;
           goto Cleanup;		   
	   }

       
       Cargo.hRoundTrip = cmsCreateTransform(p ->OutputProfile, TYPE_CMYK_16, 
                                             p ->OutputProfile, TYPE_CMYK_16, p->Intent, cmsFLAGS_NOTPRECALC);


       
       Cargo.hProofOutput  = cmsCreateTransform(p ->OutputProfile, TYPE_CMYK_16, 
                                            hLab, TYPE_Lab_DBL, p->Intent, LocalFlags);


       
       Grid =  cmsAllocLUT();
       if (!Grid) goto Cleanup;

       Grid = cmsAlloc3DGrid(Grid, nGridPoints, 4, 4);

       
       p -> FromInput = _cmsIdentifyInputFormat(p,  TYPE_CMYK_16);
       p -> ToOutput  = _cmsIdentifyOutputFormat(p, TYPE_CMYK_16);



       
       Cargo.MaxTAC = 0;
       if (!cmsSample3DGrid(Grid, EstimateTAC, (LPVOID) &Cargo, 0)) {

                cmsFreeLUT(Grid);
                Grid = NULL;
                goto Cleanup;
       }

	   
       
       if (!cmsSample3DGrid(Grid, _cmsGetBlackPreservationSampler(), (LPVOID) &Cargo, 0)) {

                cmsFreeLUT(Grid);
                Grid = NULL;
                goto Cleanup;
       }
      
       
        Cargo.MaxError = 0;
        cmsSample3DGrid(Grid, BlackPreservingEstimateErrorSampler, (LPVOID) &Cargo, SAMPLER_INSPECT);
       

Cleanup:

       if (Cargo.cmyk2Lab) cmsDeleteTransform(Cargo.cmyk2Lab);
       if (Cargo.hRoundTrip) cmsDeleteTransform(Cargo.hRoundTrip);
       if (Cargo.hProofOutput) cmsDeleteTransform(Cargo.hProofOutput);

       if (hLab) cmsCloseProfile(hLab);
       if (Cargo.KTone) cmsFreeGamma(Cargo.KTone);
       if (Cargo.LabK2cmyk) cmsFreeLUT(Cargo.LabK2cmyk);
      
       return Grid;
}





static
void PatchLUT(LPLUT Grid, WORD At[], WORD Value[],
                     int nChannelsOut, int nChannelsIn)
{
       LPL16PARAMS p16  = &Grid -> CLut16params;
       double     px, py, pz, pw;
       int        x0, y0, z0, w0;
       int        i, index;


       if (Grid ->wFlags & LUT_HASTL1) return;  

       px = ((double) At[0] * (p16->Domain)) / 65535.0;
       py = ((double) At[1] * (p16->Domain)) / 65535.0;
       pz = ((double) At[2] * (p16->Domain)) / 65535.0;
       pw = ((double) At[3] * (p16->Domain)) / 65535.0;

       x0 = (int) floor(px);
       y0 = (int) floor(py);
       z0 = (int) floor(pz);
       w0 = (int) floor(pw);

       if (nChannelsIn == 4) {

              if (((px - x0) != 0) ||
                  ((py - y0) != 0) ||
                  ((pz - z0) != 0) ||
                  ((pw - w0) != 0)) return; 

              index = p16 -> opta4 * x0 +
                      p16 -> opta3 * y0 +
                      p16 -> opta2 * z0 +
                      p16 -> opta1 * w0;
       }
       else 
       if (nChannelsIn == 3) {

              if (((px - x0) != 0) ||
                  ((py - y0) != 0) ||
                  ((pz - z0) != 0)) return;  

              index = p16 -> opta3 * x0 +
                      p16 -> opta2 * y0 +
                      p16 -> opta1 * z0;
       }
       else 
       if (nChannelsIn == 1) {

              if (((px - x0) != 0)) return; 
                          
              index = p16 -> opta1 * x0;    
       }
       else {
           cmsSignalError(LCMS_ERRC_ABORTED, "(internal) %d Channels are not supported on PatchLUT", nChannelsIn);
           return;
       }

       for (i=0; i < nChannelsOut; i++)
              Grid -> T[index + i] = Value[i];

}



LCMSBOOL _cmsFixWhiteMisalignment(_LPcmsTRANSFORM p)
{

       WORD *WhitePointIn, *WhitePointOut, *BlackPointIn, *BlackPointOut;
       int nOuts, nIns;


       if (!p -> DeviceLink) return FALSE;
       
       if (p ->Intent == INTENT_ABSOLUTE_COLORIMETRIC) return FALSE;
       if ((p ->PreviewProfile != NULL) && 
           (p ->ProofIntent == INTENT_ABSOLUTE_COLORIMETRIC)) return FALSE;


       if (!_cmsEndPointsBySpace(p -> EntryColorSpace,
                                 &WhitePointIn, &BlackPointIn, &nIns)) return FALSE;
       

       if (!_cmsEndPointsBySpace(p -> ExitColorSpace,
                                   &WhitePointOut, &BlackPointOut, &nOuts)) return FALSE;
       
       

       PatchLUT(p -> DeviceLink, WhitePointIn, WhitePointOut, nOuts, nIns);
       

       return TRUE;
}

