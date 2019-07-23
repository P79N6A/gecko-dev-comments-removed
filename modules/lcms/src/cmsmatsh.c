






















#include "lcms.h"




























static
int ComputeTables(LPGAMMATABLE Table[3], LPWORD Out[3], LPL16PARAMS p16)
{
    int i, AllLinear;

       cmsCalcL16Params(Table[0] -> nEntries, p16);

       AllLinear = 0;
       for (i=0; i < 3; i++)
       {
        LPWORD PtrW;

        PtrW = (LPWORD) _cmsMalloc(sizeof(WORD) * p16 -> nSamples);

        if (PtrW == NULL) return -1;  
              
        CopyMemory(PtrW, Table[i] -> GammaTable, sizeof(WORD) * Table[i] -> nEntries);

        Out[i] = PtrW;      

        

        AllLinear   += cmsIsLinear(PtrW, p16 -> nSamples);
       }

       
       
       
       

       if (AllLinear != 3) return 1;

       return 0;

}


LPMATSHAPER cmsAllocMatShaper2(LPMAT3 Matrix, LPGAMMATABLE In[], LPGAMMATABLE Out[], DWORD Behaviour)
{
       LPMATSHAPER NewMatShaper;
       int rc;

       NewMatShaper = (LPMATSHAPER) _cmsMalloc(sizeof(MATSHAPER));
       if (NewMatShaper)
              ZeroMemory(NewMatShaper, sizeof(MATSHAPER));

       NewMatShaper->dwFlags = Behaviour & (MATSHAPER_ALLSMELTED);

       

       MAT3toFix(&NewMatShaper -> Matrix, Matrix);

       

       if (!MAT3isIdentity(&NewMatShaper -> Matrix, 0.00001))
                     NewMatShaper -> dwFlags |= MATSHAPER_HASMATRIX;

       

       if (Out) {

            rc = ComputeTables(Out, NewMatShaper ->L, &NewMatShaper ->p16);
            if (rc < 0) {
                 cmsFreeMatShaper(NewMatShaper);
                 return NULL;
            }
            if (rc == 1) NewMatShaper -> dwFlags |= MATSHAPER_HASSHAPER;        
       }


       if (In) {

            rc = ComputeTables(In, NewMatShaper ->L2, &NewMatShaper ->p2_16);
            if (rc < 0) {
                cmsFreeMatShaper(NewMatShaper);
                return NULL;
            }
            if (rc == 1) NewMatShaper -> dwFlags |= MATSHAPER_HASINPSHAPER;     
       }

       
       return NewMatShaper;

}





LPMATSHAPER cmsAllocMatShaper(LPMAT3 Matrix, LPGAMMATABLE Tables[], DWORD Behaviour)
{
       LPMATSHAPER NewMatShaper;
       int i, AllLinear;

       NewMatShaper = (LPMATSHAPER) _cmsMalloc(sizeof(MATSHAPER));
       if (NewMatShaper)
              ZeroMemory(NewMatShaper, sizeof(MATSHAPER));

       NewMatShaper->dwFlags = Behaviour & (MATSHAPER_ALLSMELTED);

       

       MAT3toFix(&NewMatShaper -> Matrix, Matrix);

       

       if (!MAT3isIdentity(&NewMatShaper -> Matrix, 0.00001))
                     NewMatShaper -> dwFlags |= MATSHAPER_HASMATRIX;

       

       cmsCalcL16Params(Tables[0] -> nEntries, &NewMatShaper -> p16);

       

       AllLinear = 0;
       for (i=0; i < 3; i++)
       {
        LPWORD PtrW;

        PtrW = (LPWORD) _cmsMalloc(sizeof(WORD) * NewMatShaper -> p16.nSamples);

        if (PtrW == NULL) {
              cmsFreeMatShaper(NewMatShaper);
              return NULL;
        }

        CopyMemory(PtrW, Tables[i] -> GammaTable,
                            sizeof(WORD) * Tables[i] -> nEntries);

        NewMatShaper -> L[i] = PtrW;      

        

        AllLinear   += cmsIsLinear(PtrW, NewMatShaper -> p16.nSamples);
       }

       
       

       if (AllLinear != 3)
              NewMatShaper -> dwFlags |= MATSHAPER_HASSHAPER;

       return NewMatShaper;
}





void cmsFreeMatShaper(LPMATSHAPER MatShaper)
{
       int i;

       if (!MatShaper) return;

       for (i=0; i < 3; i++)
       {
              if (MatShaper -> L[i]) _cmsFree(MatShaper ->L[i]);
              if (MatShaper -> L2[i]) _cmsFree(MatShaper ->L2[i]);
       }

       _cmsFree(MatShaper);
}




static
void AllSmeltedBehaviour(LPMATSHAPER MatShaper, WORD In[], WORD Out[])
{

       WORD tmp[3];
       WVEC3 InVect, OutVect;

       if (MatShaper -> dwFlags & MATSHAPER_HASINPSHAPER)
       {
       InVect.n[VX] = cmsLinearInterpFixed(In[0], MatShaper -> L2[0], &MatShaper -> p2_16);
       InVect.n[VY] = cmsLinearInterpFixed(In[1], MatShaper -> L2[1], &MatShaper -> p2_16);
       InVect.n[VZ] = cmsLinearInterpFixed(In[2], MatShaper -> L2[2], &MatShaper -> p2_16);
       }
       else
       {
            InVect.n[VX] = ToFixedDomain(In[0]);
            InVect.n[VY] = ToFixedDomain(In[1]);
            InVect.n[VZ] = ToFixedDomain(In[2]);
       }


       if (MatShaper -> dwFlags & MATSHAPER_HASMATRIX)
       {       
                         
             MAT3evalW(&OutVect, &MatShaper -> Matrix, &InVect);
       }
       else {

           OutVect.n[VX] = InVect.n[VX];
           OutVect.n[VY] = InVect.n[VY];
           OutVect.n[VZ] = InVect.n[VZ];
       }

             
       tmp[0] = _cmsClampWord(FromFixedDomain(OutVect.n[VX]));
       tmp[1] = _cmsClampWord(FromFixedDomain(OutVect.n[VY]));
       tmp[2] = _cmsClampWord(FromFixedDomain(OutVect.n[VZ]));

       
           
       if (MatShaper -> dwFlags & MATSHAPER_HASSHAPER)
       {
       Out[0] = cmsLinearInterpLUT16(tmp[0], MatShaper -> L[0], &MatShaper -> p16);
       Out[1] = cmsLinearInterpLUT16(tmp[1], MatShaper -> L[1], &MatShaper -> p16);
       Out[2] = cmsLinearInterpLUT16(tmp[2], MatShaper -> L[2], &MatShaper -> p16);
       }
       else
       {
           Out[0] = tmp[0];
           Out[1] = tmp[1];
           Out[2] = tmp[2];
       }
        
}


static
void InputBehaviour(LPMATSHAPER MatShaper, WORD In[], WORD Out[])
{
       WVEC3 InVect, OutVect;

       
       if (MatShaper -> dwFlags & MATSHAPER_HASSHAPER)
       {
       InVect.n[VX] = cmsLinearInterpFixed(In[0], MatShaper -> L[0], &MatShaper -> p16);
       InVect.n[VY] = cmsLinearInterpFixed(In[1], MatShaper -> L[1], &MatShaper -> p16);
       InVect.n[VZ] = cmsLinearInterpFixed(In[2], MatShaper -> L[2], &MatShaper -> p16);
       }
       else
       {
       InVect.n[VX] = ToFixedDomain(In[0]);
       InVect.n[VY] = ToFixedDomain(In[1]);
       InVect.n[VZ] = ToFixedDomain(In[2]);
       }

       if (MatShaper -> dwFlags & MATSHAPER_HASMATRIX)
       {
              MAT3evalW(&OutVect, &MatShaper -> Matrix, &InVect);
       }
       else
       {
       OutVect =  InVect;
       }

       

       Out[0] = _cmsClampWord((OutVect.n[VX]) >> 1);
       Out[1] = _cmsClampWord((OutVect.n[VY]) >> 1);
       Out[2] = _cmsClampWord((OutVect.n[VZ]) >> 1);

}


static
void OutputBehaviour(LPMATSHAPER MatShaper, WORD In[], WORD Out[])
{
       WVEC3 InVect, OutVect;
       int i;

       
       

       InVect.n[VX] = (Fixed32) In[0] << 1;
       InVect.n[VY] = (Fixed32) In[1] << 1;
       InVect.n[VZ] = (Fixed32) In[2] << 1;

       if (MatShaper -> dwFlags & MATSHAPER_HASMATRIX)
       {
              MAT3evalW(&OutVect, &MatShaper -> Matrix, &InVect);
       }
       else
       {
       OutVect = InVect;
       }


       if (MatShaper -> dwFlags & MATSHAPER_HASSHAPER)
       {
              for (i=0; i < 3; i++)
              {

              Out[i] = cmsLinearInterpLUT16(
                     _cmsClampWord(FromFixedDomain(OutVect.n[i])),
                     MatShaper -> L[i],
                     &MatShaper ->p16);
              }
       }
       else
       {
       

       Out[0] = _cmsClampWord(FromFixedDomain(OutVect.n[VX]));
       Out[1] = _cmsClampWord(FromFixedDomain(OutVect.n[VY]));
       Out[2] = _cmsClampWord(FromFixedDomain(OutVect.n[VZ]));
       }

}




void cmsEvalMatShaper(LPMATSHAPER MatShaper, WORD In[], WORD Out[])
{

       if ((MatShaper -> dwFlags & MATSHAPER_ALLSMELTED) == MATSHAPER_ALLSMELTED)
       {
              AllSmeltedBehaviour(MatShaper, In, Out);
              return;
       }
       if (MatShaper -> dwFlags & MATSHAPER_INPUT)
       {
              InputBehaviour(MatShaper, In, Out);
              return;
       }

       OutputBehaviour(MatShaper, In, Out);
}
