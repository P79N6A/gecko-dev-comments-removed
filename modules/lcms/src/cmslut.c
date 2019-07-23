





















#include "lcms.h"


























































#define FROM_V2_TO_V4(x) (((((x)<<8)+(x))+0x80)>>8)    // BY 65535 DIV 65280 ROUND
#define FROM_V4_TO_V2(x) ((((x)<<8)+0x80)/257)         // BY 65280 DIV 65535 ROUND




LPLUT LCMSEXPORT cmsAllocLUT(void)
{
       LPLUT NewLUT;

       NewLUT = (LPLUT) _cmsMalloc(sizeof(LUT));
       if (NewLUT)
              ZeroMemory(NewLUT, sizeof(LUT));

       return NewLUT;
}

void LCMSEXPORT cmsFreeLUT(LPLUT Lut)
{
       unsigned int i;

       if (!Lut) return;

       if (Lut -> T) free(Lut -> T);

       for (i=0; i < Lut -> OutputChan; i++)
       {
              if (Lut -> L2[i]) free(Lut -> L2[i]);
       }

       for (i=0; i < Lut -> InputChan; i++)
       {

              if (Lut -> L1[i]) free(Lut -> L1[i]);
       }


       if (Lut ->wFlags & LUT_HASTL3) {

            for (i=0; i < Lut -> InputChan; i++) {

              if (Lut -> L3[i]) free(Lut -> L3[i]);
            }
       }

       if (Lut ->wFlags & LUT_HASTL4) {

            for (i=0; i < Lut -> OutputChan; i++) {

              if (Lut -> L4[i]) free(Lut -> L4[i]);
            }
       }

       if (Lut ->CLut16params.p8)
           free(Lut ->CLut16params.p8);

       free(Lut);
}


static
LPVOID DupBlockTab(LPVOID Org, size_t size)
{
    LPVOID mem = _cmsMalloc(size);
    if (mem != NULL)
        CopyMemory(mem, Org, size);

    return mem;
}


LPLUT LCMSEXPORT cmsDupLUT(LPLUT Orig)
{
    LPLUT NewLUT = cmsAllocLUT();
    unsigned int i;
    
       CopyMemory(NewLUT, Orig, sizeof(LUT));

       for (i=0; i < Orig ->InputChan; i++) 
            NewLUT -> L1[i] = (LPWORD) DupBlockTab((LPVOID) Orig ->L1[i], 
                                        sizeof(WORD) * Orig ->In16params.nSamples);

       for (i=0; i < Orig ->OutputChan; i++)
            NewLUT -> L2[i] = (LPWORD) DupBlockTab((LPVOID) Orig ->L2[i], 
                                        sizeof(WORD) * Orig ->Out16params.nSamples);   
       
       NewLUT -> T = (LPWORD) DupBlockTab((LPVOID) Orig ->T, Orig -> Tsize);

       return NewLUT;
}


static
unsigned int UIpow(unsigned int a, unsigned int b)
{
        unsigned int rv = 1;

        for (; b > 0; b--)
                rv *= a;

        return rv;
}


LPLUT LCMSEXPORT cmsAlloc3DGrid(LPLUT NewLUT, int clutPoints, int inputChan, int outputChan)
{
    DWORD nTabSize;

       NewLUT -> wFlags       |= LUT_HAS3DGRID;  
       NewLUT -> cLutPoints    = clutPoints;
       NewLUT -> InputChan     = inputChan;
       NewLUT -> OutputChan    = outputChan;


       nTabSize = (NewLUT -> OutputChan * UIpow(NewLUT->cLutPoints,
                                                NewLUT->InputChan)
                                                * sizeof(WORD));

       NewLUT -> T = (LPWORD) _cmsMalloc(nTabSize);
       if (NewLUT -> T == NULL) return NULL;

       ZeroMemory(NewLUT -> T, nTabSize);
       NewLUT ->Tsize = nTabSize;
       

       cmsCalcCLUT16Params(NewLUT -> cLutPoints,  NewLUT -> InputChan,
                                                  NewLUT -> OutputChan,
                                                  &NewLUT -> CLut16params);

       return NewLUT;
}




LPLUT LCMSEXPORT cmsAllocLinearTable(LPLUT NewLUT, LPGAMMATABLE Tables[], int nTable)
{
       unsigned int i;
       LPWORD PtrW;

       switch (nTable) {


       case 1: NewLUT -> wFlags |= LUT_HASTL1;
               cmsCalcL16Params(Tables[0] -> nEntries, &NewLUT -> In16params);
               NewLUT -> InputEntries = Tables[0] -> nEntries;

               for (i=0; i < NewLUT -> InputChan; i++) {

                     PtrW = (LPWORD) _cmsMalloc(sizeof(WORD) * NewLUT -> InputEntries);
                     if (PtrW == NULL) return NULL;

                     NewLUT -> L1[i] = PtrW;
                     CopyMemory(PtrW, Tables[i]->GammaTable, sizeof(WORD) * NewLUT -> InputEntries);
					 CopyMemory(&NewLUT -> LCurvesSeed[0][i], &Tables[i] -> Seed, sizeof(LCMSGAMMAPARAMS));
               }
			   

               break;

       case 2: NewLUT -> wFlags |= LUT_HASTL2;
               cmsCalcL16Params(Tables[0] -> nEntries, &NewLUT -> Out16params);
               NewLUT -> OutputEntries = Tables[0] -> nEntries;
               for (i=0; i < NewLUT -> OutputChan; i++) {

                     PtrW = (LPWORD) _cmsMalloc(sizeof(WORD) * NewLUT -> OutputEntries);
                     if (PtrW == NULL) return NULL;

                     NewLUT -> L2[i] = PtrW;
                     CopyMemory(PtrW, Tables[i]->GammaTable, sizeof(WORD) * NewLUT -> OutputEntries);
					 CopyMemory(&NewLUT -> LCurvesSeed[1][i], &Tables[i] -> Seed, sizeof(LCMSGAMMAPARAMS));
               }
               break;


       

       case 3:
               NewLUT -> wFlags |= LUT_HASTL3;
               cmsCalcL16Params(Tables[0] -> nEntries, &NewLUT -> L3params);
               NewLUT -> L3Entries = Tables[0] -> nEntries;

               for (i=0; i < NewLUT -> InputChan; i++) {

                     PtrW = (LPWORD) _cmsMalloc(sizeof(WORD) * NewLUT -> L3Entries);
                     if (PtrW == NULL) return NULL;

                     NewLUT -> L3[i] = PtrW;
                     CopyMemory(PtrW, Tables[i]->GammaTable, sizeof(WORD) * NewLUT -> L3Entries);
					 CopyMemory(&NewLUT -> LCurvesSeed[2][i], &Tables[i] -> Seed, sizeof(LCMSGAMMAPARAMS));
               }
               break;

       case 4:
               NewLUT -> wFlags |= LUT_HASTL4;
               cmsCalcL16Params(Tables[0] -> nEntries, &NewLUT -> L4params);
               NewLUT -> L4Entries = Tables[0] -> nEntries;
               for (i=0; i < NewLUT -> OutputChan; i++) {

                     PtrW = (LPWORD) _cmsMalloc(sizeof(WORD) * NewLUT -> L4Entries);
                     if (PtrW == NULL) return NULL;

                     NewLUT -> L4[i] = PtrW;
                     CopyMemory(PtrW, Tables[i]->GammaTable, sizeof(WORD) * NewLUT -> L4Entries);
					 CopyMemory(&NewLUT -> LCurvesSeed[3][i], &Tables[i] -> Seed, sizeof(LCMSGAMMAPARAMS));
               }
               break;
               

       default:;
       }

       return NewLUT;
}




LPLUT LCMSEXPORT cmsSetMatrixLUT(LPLUT Lut, LPMAT3 M)
{
        MAT3toFix(&Lut ->Matrix, M);

        if (!MAT3isIdentity(&Lut->Matrix, 0.0001))
            Lut ->wFlags |= LUT_HASMATRIX;

        return Lut;
}




LPLUT LCMSEXPORT cmsSetMatrixLUT4(LPLUT Lut, LPMAT3 M, LPVEC3 off, DWORD dwFlags)
{
    WMAT3 WMat;
    WVEC3 Woff;
    VEC3  Zero = {{0, 0, 0}};

        MAT3toFix(&WMat, M);

        if (off == NULL)
                off = &Zero;

        VEC3toFix(&Woff, off);

        
        if (MAT3isIdentity(&WMat, 0.0001) && 
            (Woff.n[VX] == 0 && Woff.n[VY] == 0 && Woff.n[VZ] == 0))
            return Lut;

        switch (dwFlags) {

        case LUT_HASMATRIX:
                Lut ->Matrix = WMat;                
                Lut ->wFlags |= LUT_HASMATRIX;
                break;

        case LUT_HASMATRIX3:
                Lut ->Mat3 = WMat;
                Lut ->Ofs3 = Woff;
                Lut ->wFlags |= LUT_HASMATRIX3;
                break;

        case LUT_HASMATRIX4:
                Lut ->Mat4 = WMat;
                Lut ->Ofs4 = Woff;
                Lut ->wFlags |= LUT_HASMATRIX4;
                break;


        default:;
        }

        return Lut;
}





void LCMSEXPORT cmsEvalLUT(LPLUT Lut, WORD In[], WORD Out[])
{
       register unsigned int i;
       WORD StageABC[MAXCHANNELS], StageLMN[MAXCHANNELS];

        
       
       if (Lut ->wFlags == LUT_HAS3DGRID) {

            Lut ->CLut16params.Interp3D(In, Out, Lut -> T, &Lut -> CLut16params);
            return;
       }
       

       

       for (i=0; i < Lut -> InputChan; i++)
                            StageABC[i] = In[i];

       
       if (Lut ->wFlags & LUT_V4_OUTPUT_EMULATE_V2) {
           
           
           if (StageABC[0] > 0xFF00)
               StageABC[0] = 0xFF00;

           StageABC[0] = (WORD) FROM_V2_TO_V4(StageABC[0]);
           StageABC[1] = (WORD) FROM_V2_TO_V4(StageABC[1]);
           StageABC[2] = (WORD) FROM_V2_TO_V4(StageABC[2]);
           
       }

       if (Lut ->wFlags & LUT_V2_OUTPUT_EMULATE_V4) {
           
           StageABC[0] = (WORD) FROM_V4_TO_V2(StageABC[0]);
           StageABC[1] = (WORD) FROM_V4_TO_V2(StageABC[1]);
           StageABC[2] = (WORD) FROM_V4_TO_V2(StageABC[2]);           
       }


       

       if (Lut -> wFlags & LUT_HASMATRIX) {

              WVEC3 InVect, OutVect;
              
              

              if (Lut ->FixGrayAxes) {

                  StageABC[1] = _cmsClampWord(StageABC[1] - 128);
                  StageABC[2] = _cmsClampWord(StageABC[2] - 128);
              }

              

              InVect.n[VX] = ToFixedDomain(StageABC[0]);
              InVect.n[VY] = ToFixedDomain(StageABC[1]);
              InVect.n[VZ] = ToFixedDomain(StageABC[2]);
              

              MAT3evalW(&OutVect, &Lut -> Matrix, &InVect);

              

              StageABC[0] = _cmsClampWord(FromFixedDomain(OutVect.n[VX]));
              StageABC[1] = _cmsClampWord(FromFixedDomain(OutVect.n[VY]));
              StageABC[2] = _cmsClampWord(FromFixedDomain(OutVect.n[VZ]));
       }
       

       

       if (Lut -> wFlags & LUT_HASTL1)
       {
              for (i=0; i < Lut -> InputChan; i++)
                     StageABC[i] = cmsLinearInterpLUT16(StageABC[i],
                                                   Lut -> L1[i],
                                                   &Lut -> In16params);
       }


       
             
       if (Lut ->wFlags & LUT_HASMATRIX3) {

              WVEC3 InVect, OutVect;

              InVect.n[VX] = ToFixedDomain(StageABC[0]);
              InVect.n[VY] = ToFixedDomain(StageABC[1]);
              InVect.n[VZ] = ToFixedDomain(StageABC[2]);

              MAT3evalW(&OutVect, &Lut -> Mat3, &InVect);              

              OutVect.n[VX] += Lut ->Ofs3.n[VX];
              OutVect.n[VY] += Lut ->Ofs3.n[VY];
              OutVect.n[VZ] += Lut ->Ofs3.n[VZ];

              StageABC[0] = _cmsClampWord(FromFixedDomain(OutVect.n[VX]));
              StageABC[1] = _cmsClampWord(FromFixedDomain(OutVect.n[VY]));
              StageABC[2] = _cmsClampWord(FromFixedDomain(OutVect.n[VZ]));

       }
       
       if (Lut ->wFlags & LUT_HASTL3) {

             for (i=0; i < Lut -> InputChan; i++)
                     StageABC[i] = cmsLinearInterpLUT16(StageABC[i],
                                                   Lut -> L3[i],
                                                   &Lut -> L3params);

       }



       if (Lut -> wFlags & LUT_HAS3DGRID) {

            Lut ->CLut16params.Interp3D(StageABC, StageLMN, Lut -> T, &Lut -> CLut16params);

       }
       else
       {              

              for (i=0; i < Lut -> InputChan; i++)
                            StageLMN[i] = StageABC[i];

       }


       
     
       if (Lut ->wFlags & LUT_HASTL4) {

            for (i=0; i < Lut -> OutputChan; i++)
                     StageLMN[i] = cmsLinearInterpLUT16(StageLMN[i],
                                                   Lut -> L4[i],
                                                   &Lut -> L4params);
       }
        
       if (Lut ->wFlags & LUT_HASMATRIX4) {

              WVEC3 InVect, OutVect;

              InVect.n[VX] = ToFixedDomain(StageLMN[0]);
              InVect.n[VY] = ToFixedDomain(StageLMN[1]);
              InVect.n[VZ] = ToFixedDomain(StageLMN[2]);

              MAT3evalW(&OutVect, &Lut -> Mat4, &InVect);              

              OutVect.n[VX] += Lut ->Ofs4.n[VX];
              OutVect.n[VY] += Lut ->Ofs4.n[VY];
              OutVect.n[VZ] += Lut ->Ofs4.n[VZ];

              StageLMN[0] = _cmsClampWord(FromFixedDomain(OutVect.n[VX]));
              StageLMN[1] = _cmsClampWord(FromFixedDomain(OutVect.n[VY]));
              StageLMN[2] = _cmsClampWord(FromFixedDomain(OutVect.n[VZ]));

       }

       

       if (Lut -> wFlags & LUT_HASTL2)
       {
              for (i=0; i < Lut -> OutputChan; i++)
                     Out[i] = cmsLinearInterpLUT16(StageLMN[i],
                                                   Lut -> L2[i],
                                                   &Lut -> Out16params);
       }
       else
       {
       for (i=0; i < Lut -> OutputChan; i++)
              Out[i] = StageLMN[i];
       }

       

       if (Lut ->wFlags & LUT_V4_INPUT_EMULATE_V2) {
           
           Out[0] = (WORD) FROM_V4_TO_V2(Out[0]);
           Out[1] = (WORD) FROM_V4_TO_V2(Out[1]);
           Out[2] = (WORD) FROM_V4_TO_V2(Out[2]);
           
       }

       if (Lut ->wFlags & LUT_V2_INPUT_EMULATE_V4) {
           
           Out[0] = (WORD) FROM_V2_TO_V4(Out[0]);
           Out[1] = (WORD) FROM_V2_TO_V4(Out[1]);
           Out[2] = (WORD) FROM_V2_TO_V4(Out[2]);           
       }
}




LPLUT _cmsBlessLUT8(LPLUT Lut)
{
   int i, j;
   WORD StageABC[3];
   Fixed32 v1, v2, v3;
   LPL8PARAMS p8; 
   LPL16PARAMS p = &Lut ->CLut16params;

  
   p8 = (LPL8PARAMS) _cmsMalloc(sizeof(L8PARAMS));
   if (p8 == NULL) return NULL;

  
  

   for (i=0; i < 256; i++) {

           StageABC[0] = StageABC[1] = StageABC[2] = RGB_8_TO_16(i);

           if (Lut ->wFlags & LUT_HASTL1) {

              for (j=0; j < 3; j++)
                     StageABC[j] = cmsLinearInterpLUT16(StageABC[j],
                                                        Lut -> L1[j],
                                                       &Lut -> In16params);
              Lut ->wFlags &= ~LUT_HASTL1;
           }
    
               
           v1 = ToFixedDomain(StageABC[0] * p -> Domain);
           v2 = ToFixedDomain(StageABC[1] * p -> Domain);
           v3 = ToFixedDomain(StageABC[2] * p -> Domain);

           p8 ->X0[i] = p->opta3 * FIXED_TO_INT(v1);
           p8 ->Y0[i] = p->opta2 * FIXED_TO_INT(v2);
           p8 ->Z0[i] = p->opta1 * FIXED_TO_INT(v3);

           p8 ->rx[i] = (WORD) FIXED_REST_TO_INT(v1);
           p8 ->ry[i] = (WORD) FIXED_REST_TO_INT(v2);
           p8 ->rz[i] = (WORD) FIXED_REST_TO_INT(v3);
  
  }

   Lut -> CLut16params.p8 = p8;
   Lut -> CLut16params.Interp3D = cmsTetrahedralInterp8;

   return Lut;

}




























#define JACOBIAN_EPSILON            0.001
#define INVERSION_MAX_ITERATIONS    30





static 
void IncDelta(double *Val)
{
    if (*Val < (1.0 - JACOBIAN_EPSILON)) 

        *Val += JACOBIAN_EPSILON;
    
    else 
        *Val -= JACOBIAN_EPSILON;
    
}



static
void ToEncoded(WORD Encoded[3], LPVEC3 Float)
{
    Encoded[0] = (WORD) floor(Float->n[0] * 65535.0 + 0.5);
    Encoded[1] = (WORD) floor(Float->n[1] * 65535.0 + 0.5);
    Encoded[2] = (WORD) floor(Float->n[2] * 65535.0 + 0.5);
}

static
void FromEncoded(LPVEC3 Float, WORD Encoded[3])
{
    Float->n[0] = Encoded[0] / 65535.0;
    Float->n[1] = Encoded[1] / 65535.0;
    Float->n[2] = Encoded[2] / 65535.0;
}


static
void EvalLUTdoubleKLab(LPLUT Lut, const VEC3* In, WORD FixedK, LPcmsCIELab Out)
{
    WORD wIn[4], wOut[3];

    wIn[0] = (WORD) floor(In ->n[0] * 65535.0 + 0.5);
    wIn[1] = (WORD) floor(In ->n[1] * 65535.0 + 0.5);
    wIn[2] = (WORD) floor(In ->n[2] * 65535.0 + 0.5);
    wIn[3] = FixedK;

    cmsEvalLUT(Lut, wIn, wOut);     
	cmsLabEncoded2Float(Out, wOut);
}



static
void ComputeJacobianLab(LPLUT Lut, LPMAT3 Jacobian, const VEC3* Colorant, WORD K)
{
    VEC3 ColorantD;
    cmsCIELab Lab, LabD;
    int  j;
            
    EvalLUTdoubleKLab(Lut, Colorant, K, &Lab);
    

    for (j = 0; j < 3; j++) {

        ColorantD.n[0] = Colorant ->n[0];
        ColorantD.n[1] = Colorant ->n[1];
        ColorantD.n[2] = Colorant ->n[2];
        
        IncDelta(&ColorantD.n[j]);

        EvalLUTdoubleKLab(Lut, &ColorantD, K, &LabD);
				
		Jacobian->v[0].n[j] = ((LabD.L - Lab.L) / JACOBIAN_EPSILON);
		Jacobian->v[1].n[j] = ((LabD.a - Lab.a) / JACOBIAN_EPSILON);
		Jacobian->v[2].n[j] = ((LabD.b - Lab.b) / JACOBIAN_EPSILON);
        
    }
}









LCMSAPI double LCMSEXPORT cmsEvalLUTreverse(LPLUT Lut, WORD Target[], WORD Result[], LPWORD Hint)
{
    int      i;
    double     error, LastError = 1E20;
    cmsCIELab  fx, Goal;
    VEC3       tmp, tmp2, x;
    MAT3       Jacobian;
    WORD       FixedK;
    WORD       LastResult[4];
    
        
    
    cmsLabEncoded2Float(&Goal, Target);
    
    

    if (Lut ->InputChan == 4)
            FixedK = Target[3];
    else
            FixedK = 0;
        
    
    

    if (Hint == NULL) {

        

        x.n[0] = x.n[1] = x.n[2] = 0.3;

    }
    else {

        FromEncoded(&x, Hint);
    }
    

    
    
    for (i = 0; i < INVERSION_MAX_ITERATIONS; i++) {

        
        EvalLUTdoubleKLab(Lut, &x, FixedK, &fx);
    
        
        error = cmsDeltaE(&fx, &Goal);
                        
        
        if (error >= LastError) 
            break;

        
        LastError = error;

        ToEncoded(LastResult, &x);
        LastResult[3] = FixedK;
                
        
        ComputeJacobianLab(Lut, &Jacobian, &x, FixedK);

		
		tmp2.n[0] = fx.L - Goal.L;
		tmp2.n[1] = fx.a - Goal.a;
		tmp2.n[2] = fx.b - Goal.b;

		if (!MAT3solve(&tmp, &Jacobian, &tmp2))
			break;
		
       	
		x.n[0] -= tmp.n[0];
	    x.n[1] -= tmp.n[1];
		x.n[2] -= tmp.n[2];
               
        
        VEC3saturate(&x);                
    }

    Result[0] = LastResult[0];
    Result[1] = LastResult[1];
    Result[2] = LastResult[2];
    Result[3] = LastResult[3];

    return LastError;    
    
}



