






















#include "lcms.h"



LPGAMMATABLE LCMSEXPORT cmsAllocGamma(int nEntries);
void         LCMSEXPORT cmsFreeGamma(LPGAMMATABLE Gamma);
void         LCMSEXPORT cmsFreeGammaTriple(LPGAMMATABLE Gamma[3]);
LPGAMMATABLE LCMSEXPORT cmsBuildGamma(int nEntries, double Gamma);
LPGAMMATABLE LCMSEXPORT cmsDupGamma(LPGAMMATABLE Src);
LPGAMMATABLE LCMSEXPORT cmsReverseGamma(int nResultSamples, LPGAMMATABLE InGamma);
LPGAMMATABLE LCMSEXPORT cmsBuildParametricGamma(int nEntries, int Type, double Params[]);
LPGAMMATABLE LCMSEXPORT cmsJoinGamma(LPGAMMATABLE InGamma, LPGAMMATABLE OutGamma);
LPGAMMATABLE LCMSEXPORT cmsJoinGammaEx(LPGAMMATABLE InGamma, LPGAMMATABLE OutGamma, int nPoints);
LCMSBOOL         LCMSEXPORT cmsSmoothGamma(LPGAMMATABLE Tab, double lambda);

LCMSBOOL         cdecl _cmsSmoothEndpoints(LPWORD Table, int nPoints);




LPSAMPLEDCURVE cdecl cmsAllocSampledCurve(int nItems);
void           cdecl cmsFreeSampledCurve(LPSAMPLEDCURVE p);
void           cdecl cmsEndpointsOfSampledCurve(LPSAMPLEDCURVE p, double* Min, double* Max);
void           cdecl cmsClampSampledCurve(LPSAMPLEDCURVE p, double Min, double Max);
LCMSBOOL       cdecl cmsSmoothSampledCurve(LPSAMPLEDCURVE Tab, double SmoothingLambda);
void           cdecl cmsRescaleSampledCurve(LPSAMPLEDCURVE p, double Min, double Max, int nPoints);

LPSAMPLEDCURVE cdecl cmsJoinSampledCurves(LPSAMPLEDCURVE X, LPSAMPLEDCURVE Y, int nResultingPoints);

double LCMSEXPORT cmsEstimateGamma(LPGAMMATABLE t);
double LCMSEXPORT cmsEstimateGammaEx(LPWORD GammaTable, int nEntries, double Thereshold);




#define MAX_KNOTS   4096
typedef float vec[MAX_KNOTS+1];





#define QUOTIENT 0x04c11db7
    
static
unsigned int Crc32(unsigned int result, LPVOID ptr, int len)
{    
    int          i,j;
    BYTE         octet; 
    LPBYTE       data = (LPBYTE) ptr;
        
    for (i=0; i < len; i++) {

        octet = *data++;

        for (j=0; j < 8; j++) {

            if (result & 0x80000000) {

                result = (result << 1) ^ QUOTIENT ^ (octet >> 7);
            }
            else
            {
                result = (result << 1) ^ (octet >> 7);
            }
            octet <<= 1;
        }
    }
    
    return result;
}



unsigned int _cmsCrc32OfGammaTable(LPGAMMATABLE Table)
{
    unsigned int crc = ~0U;

    crc = Crc32(crc, &Table -> Seed.Type,  sizeof(int));
    crc = Crc32(crc, Table ->Seed.Params,  sizeof(double)*10);
    crc = Crc32(crc, &Table ->nEntries,    sizeof(int));
    crc = Crc32(crc, Table ->GammaTable,   sizeof(WORD) * Table -> nEntries);

    return ~crc;

}


LPGAMMATABLE LCMSEXPORT cmsAllocGamma(int nEntries)
{
       LPGAMMATABLE p;
       size_t size;

       if (nEntries > 65530 || nEntries < 0) {
                cmsSignalError(LCMS_ERRC_ABORTED, "Couldn't create gammatable of more than 65530 entries");
                return NULL;
       }

       size = sizeof(GAMMATABLE) + (sizeof(WORD) * (nEntries-1));

       p = (LPGAMMATABLE) _cmsMalloc(size);
       if (!p) return NULL;

       ZeroMemory(p, size);

       p -> Seed.Type     = 0;
       p -> nEntries = nEntries;
       
       return p;
}

void LCMSEXPORT cmsFreeGamma(LPGAMMATABLE Gamma)
{
       if (Gamma)  _cmsFree(Gamma);
}



void LCMSEXPORT cmsFreeGammaTriple(LPGAMMATABLE Gamma[3])
{
    cmsFreeGamma(Gamma[0]);
    cmsFreeGamma(Gamma[1]);
    cmsFreeGamma(Gamma[2]);
    Gamma[0] = Gamma[1] = Gamma[2] = NULL;
}





LPGAMMATABLE  LCMSEXPORT cmsDupGamma(LPGAMMATABLE In)
{
       LPGAMMATABLE Ptr;
       size_t size;

       Ptr = cmsAllocGamma(In -> nEntries);
       if (Ptr == NULL) return NULL;

       size = sizeof(GAMMATABLE) + (sizeof(WORD) * (In -> nEntries-1));

       CopyMemory(Ptr, In, size);                  
       return Ptr;
}





LPGAMMATABLE LCMSEXPORT cmsJoinGamma(LPGAMMATABLE InGamma,
                          LPGAMMATABLE OutGamma)
{
       register int i;
       L16PARAMS L16In, L16Out;
       LPWORD InPtr, OutPtr;
       LPGAMMATABLE p;

       p = cmsAllocGamma(256);
       if (!p) return NULL;

       cmsCalcL16Params(InGamma -> nEntries, &L16In);
       InPtr  = InGamma -> GammaTable;

       cmsCalcL16Params(OutGamma -> nEntries, &L16Out);
       OutPtr = OutGamma-> GammaTable;

       for (i=0; i < 256; i++)
       {
              WORD wValIn, wValOut;

              wValIn  = cmsLinearInterpLUT16(RGB_8_TO_16(i), InPtr, &L16In);
              wValOut = cmsReverseLinearInterpLUT16(wValIn, OutPtr, &L16Out);
              
              p -> GammaTable[i] = wValOut;
       }

       return p;
}














LPGAMMATABLE LCMSEXPORT cmsJoinGammaEx(LPGAMMATABLE InGamma,
                                       LPGAMMATABLE OutGamma, int nPoints)
{

    LPSAMPLEDCURVE x, y, r;
    LPGAMMATABLE res;
    
    x = cmsConvertGammaToSampledCurve(InGamma,  nPoints);
    y = cmsConvertGammaToSampledCurve(OutGamma, nPoints);
    r = cmsJoinSampledCurves(y, x, nPoints);

    
    cmsSmoothSampledCurve(r, 0.001);

    cmsClampSampledCurve(r, 0.0, 65535.0); 
    
    cmsFreeSampledCurve(x);
    cmsFreeSampledCurve(y);

    res = cmsConvertSampledCurveToGamma(r, 65535.0);
    cmsFreeSampledCurve(r);

    return res;
}





LPGAMMATABLE LCMSEXPORT cmsReverseGamma(int nResultSamples, LPGAMMATABLE InGamma)
{
       register int i;
       L16PARAMS L16In;
       LPWORD InPtr;
       LPGAMMATABLE p;

       
       if (InGamma -> Seed.Type > 0 && InGamma -> Seed.Type <= 5 &&				
			_cmsCrc32OfGammaTable(InGamma) == InGamma -> Seed.Crc32) {

                return cmsBuildParametricGamma(nResultSamples, -(InGamma -> Seed.Type), InGamma ->Seed.Params);
       }


       
       p = cmsAllocGamma(nResultSamples);
       if (!p) return NULL;

       cmsCalcL16Params(InGamma -> nEntries, &L16In);
       InPtr  = InGamma -> GammaTable;

       for (i=0; i < nResultSamples; i++)
       {
              WORD wValIn, wValOut;

              wValIn = _cmsQuantizeVal(i, nResultSamples);           
              wValOut = cmsReverseLinearInterpLUT16(wValIn, InPtr, &L16In);
              p -> GammaTable[i] = wValOut;
       }

           
       return p;
}









LPGAMMATABLE LCMSEXPORT cmsBuildParametricGamma(int nEntries, int Type, double Params[])
{
        LPGAMMATABLE Table;
        double R, Val, dval, e;
        int i;
        int ParamsByType[] = { 0, 1, 3, 4, 5, 7 };

        Table = cmsAllocGamma(nEntries);
        if (NULL == Table) return NULL;

        Table -> Seed.Type = Type;       

        CopyMemory(Table ->Seed.Params, Params, ParamsByType[abs(Type)] * sizeof(double));


        for (i=0; i < nEntries; i++) {

                R   = (double) i / (nEntries-1);

                switch (Type) {

                
                case 1:
                      Val = pow(R, Params[0]);
                      break;

                
                case -1:
                      Val = pow(R, 1/Params[0]);
                      break;

                
                
                
                case 2:
                    if (R >= -Params[2] / Params[1]) {
                              
                              e = Params[1]*R + Params[2];

                              if (e > 0)
                                Val = pow(e, Params[0]);
                              else
                                Val = 0;
                    }
                    else
                              Val = 0;
                      break;

                
                
                case -2: 
                    
                    Val = (pow(R, 1.0/Params[0]) - Params[2]) / Params[1];
                    if (Val < 0)
                            Val = 0;                            
                    break;


                
                
                
                case 3:
                    if (R >= -Params[2] / Params[1]) {
                            
                      e = Params[1]*R + Params[2];                    
                      Val = pow(e, Params[0]) + Params[3];
                    }
                    else
                      Val = Params[3];
                    break;


                
                
                
                    
                case -3:
                    if (R >= Params[3])  {
                        e = R - Params[3];
                        Val = (pow(e, 1/Params[0]) - Params[2]) / Params[1];
                        if (Val < 0) Val = 0;
                    }
                    else {
                        Val = -Params[2] / Params[1];
                    }
                    break;


                
                
                
                case 4:
                    if (R >= Params[4]) {
                              
                              e = Params[1]*R + Params[2];
                              if (e > 0)
                                Val = pow(e, Params[0]);
                              else
                                Val = 0;
                    }
                      else
                              Val = R * Params[3];
                      break;

                
                
                

                case -4:
                    if (R >= pow(Params[1] * Params[4] + Params[2], Params[0])) {

                        Val = (pow(R, 1.0/Params[0]) - Params[2]) / Params[1];
                    }
                    else {
                        Val = R / Params[3];
                    }
                    break;
                


                
                
                case 5:
                    if (R >= Params[4]) {
                             
                        e = Params[1]*R + Params[2];
                        Val = pow(e, Params[0]) + Params[5];
                    }        
                    else
                        Val = R*Params[3] + Params[6];
                    break;


                
                
                
                case -5:

                if (R >= pow(Params[1] * Params[4], Params[0]) + Params[5]) {

                    Val = pow(R - Params[5], 1/Params[0]) - Params[2] / Params[1];
                }
                else {
                    Val = (R - Params[6]) / Params[3];
                }
                break;

                default:
                        cmsSignalError(LCMS_ERRC_ABORTED, "Unsupported parametric curve type=%d", abs(Type)-1);
                        cmsFreeGamma(Table);
                        return NULL;
                }


        

        dval = Val * 65535.0 + .5;
        if (dval > 65535.) dval = 65535.0;
        if (dval < 0) dval = 0;

        Table->GammaTable[i] = (WORD) floor(dval);
        }

        Table -> Seed.Crc32 = _cmsCrc32OfGammaTable(Table);

        return Table;
}



LPGAMMATABLE LCMSEXPORT cmsBuildGamma(int nEntries, double Gamma)
{
    return cmsBuildParametricGamma(nEntries, 1, &Gamma);
}













static
void smooth2(vec w, vec y, vec z, float lambda, int m)
{
  int i, i1, i2;
  vec c, d, e;
  d[1] = w[1] + lambda;
  c[1] = -2 * lambda / d[1];
  e[1] = lambda /d[1];
  z[1] = w[1] * y[1];
  d[2] = w[2] + 5 * lambda - d[1] * c[1] *  c[1];
  c[2] = (-4 * lambda - d[1] * c[1] * e[1]) / d[2];
  e[2] = lambda / d[2];
  z[2] = w[2] * y[2] - c[1] * z[1];
  for (i = 3; i < m - 1; i++) {
    i1 = i - 1; i2 = i - 2;
    d[i]= w[i] + 6 * lambda - c[i1] * c[i1] * d[i1] - e[i2] * e[i2] * d[i2];
    c[i] = (-4 * lambda -d[i1] * c[i1] * e[i1])/ d[i];
    e[i] = lambda / d[i];
    z[i] = w[i] * y[i] - c[i1] * z[i1] - e[i2] * z[i2];
  }
  i1 = m - 2; i2 = m - 3;
  d[m - 1] = w[m - 1] + 5 * lambda -c[i1] * c[i1] * d[i1] - e[i2] * e[i2] * d[i2];
  c[m - 1] = (-2 * lambda - d[i1] * c[i1] * e[i1]) / d[m - 1];
  z[m - 1] = w[m - 1] * y[m - 1] - c[i1] * z[i1] - e[i2] * z[i2];
  i1 = m - 1; i2 = m - 2;
  d[m] = w[m] + lambda - c[i1] * c[i1] * d[i1] - e[i2] * e[i2] * d[i2];
  z[m] = (w[m] * y[m] - c[i1] * z[i1] - e[i2] * z[i2]) / d[m];
  z[m - 1] = z[m - 1] / d[m - 1] - c[m - 1] * z[m];
  for (i = m - 2; 1<= i; i--)
     z[i] = z[i] / d[i] - c[i] * z[i + 1] - e[i] * z[i + 2];
}





LCMSBOOL LCMSEXPORT cmsSmoothGamma(LPGAMMATABLE Tab, double lambda)

{
    vec w, y, z;
    int i, nItems, Zeros, Poles;


    if (cmsIsLinear(Tab->GammaTable, Tab->nEntries)) return FALSE; 

    nItems = Tab -> nEntries;

    if (nItems > MAX_KNOTS) {
                cmsSignalError(LCMS_ERRC_ABORTED, "cmsSmoothGamma: too many points.");
                return FALSE;
                }

    ZeroMemory(w, nItems * sizeof(float));
    ZeroMemory(y, nItems * sizeof(float));
    ZeroMemory(z, nItems * sizeof(float));

    for (i=0; i < nItems; i++)
    {
        y[i+1] = (float) Tab -> GammaTable[i];
        w[i+1] = 1.0;
    }

    smooth2(w, y, z, (float) lambda, nItems);

    
    Zeros = Poles = 0;
    for (i=nItems; i > 1; --i) {

            if (z[i] == 0.) Zeros++;
            if (z[i] >= 65535.) Poles++;
            if (z[i] < z[i-1]) return FALSE; 
    }

    if (Zeros > (nItems / 3)) return FALSE;  
    if (Poles > (nItems / 3)) return FALSE;  

    

    for (i=0; i < nItems; i++) {

        

        float v = z[i+1];

        if (v < 0) v = 0;
        if (v > 65535.) v = 65535.;

        Tab -> GammaTable[i] = (WORD) floor(v + .5);
        }

    return TRUE;
}




double LCMSEXPORT cmsEstimateGammaEx(LPWORD GammaTable, int nEntries, double Thereshold)
{
    double gamma, sum, sum2;
    double n, x, y, Std;
    int i;

    sum = sum2 = n = 0;
    
    
    for (i=1; i < nEntries - 1; i++) {

            x = (double) i / (nEntries - 1);
            y = (double) GammaTable[i] / 65535.;
            
            
            

            if (y > 0. && y < 1. && x > 0.07) {

            gamma = log(y) / log(x);
            sum  += gamma;
            sum2 += gamma * gamma;
            n++;
            }
            
    }

    
    Std = sqrt((n * sum2 - sum * sum) / (n*(n-1)));
    

    if (Std > Thereshold)
        return -1.0;

    return (sum / n);   
}


double LCMSEXPORT cmsEstimateGamma(LPGAMMATABLE t)
{
        return cmsEstimateGammaEx(t->GammaTable, t->nEntries, 0.7);
}






LPSAMPLEDCURVE cmsAllocSampledCurve(int nItems)
{
    LPSAMPLEDCURVE pOut;

    pOut = (LPSAMPLEDCURVE) _cmsMalloc(sizeof(SAMPLEDCURVE));
    if (pOut == NULL)
            return NULL;

    if((pOut->Values = (double *) _cmsMalloc(nItems * sizeof(double))) == NULL)
    {
         _cmsFree(pOut);
        return NULL;
    }

    pOut->nItems = nItems;
    ZeroMemory(pOut->Values, nItems * sizeof(double));

    return pOut;
}


void cmsFreeSampledCurve(LPSAMPLEDCURVE p)
{
     _cmsFree((LPVOID) p -> Values);
     _cmsFree((LPVOID) p);
}





LPSAMPLEDCURVE cmsDupSampledCurve(LPSAMPLEDCURVE p)
{
    LPSAMPLEDCURVE out;

    out = cmsAllocSampledCurve(p -> nItems);
    if (!out) return NULL;

    CopyMemory(out ->Values, p ->Values, p->nItems * sizeof(double));

    return out;
}




void cmsEndpointsOfSampledCurve(LPSAMPLEDCURVE p, double* Min, double* Max)
{
        int i;

        *Min = 65536.;
        *Max = 0.;

        for (i=0; i < p -> nItems; i++) {

                double v = p -> Values[i];

                if (v < *Min)
                        *Min = v;

                if (v > *Max)
                        *Max = v;
        }

        if (*Min < 0) *Min = 0;
        if (*Max > 65535.0) *Max = 65535.0;
}



void cmsClampSampledCurve(LPSAMPLEDCURVE p, double Min, double Max)
{

        int i;

        for (i=0; i < p -> nItems; i++) {

                double v = p -> Values[i];

                if (v < Min)
                        v = Min;

                if (v > Max)
                        v = Max;

                p -> Values[i] = v;

        }

}





LCMSBOOL cmsSmoothSampledCurve(LPSAMPLEDCURVE Tab, double lambda)
{
    vec w, y, z;
    int i, nItems;

    nItems = Tab -> nItems;

    if (nItems > MAX_KNOTS) {
                cmsSignalError(LCMS_ERRC_ABORTED, "cmsSmoothSampledCurve: too many points.");
                return FALSE;
                }

    ZeroMemory(w, nItems * sizeof(float));
    ZeroMemory(y, nItems * sizeof(float));
    ZeroMemory(z, nItems * sizeof(float));

    for (i=0; i < nItems; i++)
    {
        float value = (float) Tab -> Values[i];

        y[i+1] = value;
        w[i+1] = (float) ((value < 0.0) ?  0 : 1);
    }


    smooth2(w, y, z, (float) lambda, nItems);

    for (i=0; i < nItems; i++) {

        Tab -> Values[i] = z[i+1];;
     }

    return TRUE;

}





static
double ScaleVal(double v, double Min, double Max, int nPoints)
{

        double a, b;

        if (v <= Min) return 0;
        if (v >= Max) return (nPoints-1);
    
        a = (double) (nPoints - 1) / (Max - Min);
        b = a * Min;

        return (a * v) - b;

}




void cmsRescaleSampledCurve(LPSAMPLEDCURVE p, double Min, double Max, int nPoints)
{

        int i;

        for (i=0; i < p -> nItems; i++) {

                double v = p -> Values[i];

                p -> Values[i] = ScaleVal(v, Min, Max, nPoints);
        }

}




LPSAMPLEDCURVE cmsJoinSampledCurves(LPSAMPLEDCURVE X, LPSAMPLEDCURVE Y, int nResultingPoints)
{
    int i, j;   
    LPSAMPLEDCURVE out;
    double MinX, MinY, MaxX, MaxY;
    double x, y, x1, y1, x2, y2, a, b;

    out = cmsAllocSampledCurve(nResultingPoints);
    if (out == NULL)
        return NULL;

    if (X -> nItems != Y -> nItems) {

        cmsSignalError(LCMS_ERRC_ABORTED, "cmsJoinSampledCurves: invalid curve.");
        cmsFreeSampledCurve(out);
        return NULL;
    }

    
    cmsEndpointsOfSampledCurve(X, &MinX, &MaxX);
    cmsEndpointsOfSampledCurve(Y, &MinY, &MaxY);

    
    
    out ->Values[0] = MinY; 
    for (i=1; i < nResultingPoints; i++) {

        
        x = (i * (MaxX - MinX) / (nResultingPoints-1)) + MinX;

        
        

        j = 1;
        while ((j < X ->nItems - 1) && X ->Values[j] < x)
            j++;
            
        
        x1 = X ->Values[j-1]; x2 = X ->Values[j];
        y1 = Y ->Values[j-1]; y2 = Y ->Values[j];

        
        a = (y1 - y2) / (x1 - x2);
        b = y1 - a * x1;
        y = a* x + b;
        
        out ->Values[i] = y;
    }
    

    cmsClampSampledCurve(out, MinY, MaxY);
    return out;
}





LPGAMMATABLE cmsConvertSampledCurveToGamma(LPSAMPLEDCURVE Sampled, double Max)
{
    LPGAMMATABLE Gamma;
    int i, nPoints;
    

    nPoints = Sampled ->nItems;

    Gamma = cmsAllocGamma(nPoints);
    for (i=0; i < nPoints; i++) {
        
        Gamma->GammaTable[i] = (WORD) floor(ScaleVal(Sampled ->Values[i], 0, Max, 65536) + .5);
    }

    return Gamma;

}



LPSAMPLEDCURVE cmsConvertGammaToSampledCurve(LPGAMMATABLE Gamma, int nPoints)
{
    LPSAMPLEDCURVE Sampled;
    L16PARAMS L16;
    int i;
    WORD wQuant, wValIn;

    if (nPoints > 4096) {

        cmsSignalError(LCMS_ERRC_ABORTED, "cmsConvertGammaToSampledCurve: too many points (max=4096)");
        return NULL;
    }

    cmsCalcL16Params(Gamma -> nEntries, &L16);
       
    Sampled = cmsAllocSampledCurve(nPoints);
    for (i=0; i < nPoints; i++) {
            wQuant  = _cmsQuantizeVal(i, nPoints);
            wValIn  = cmsLinearInterpLUT16(wQuant, Gamma ->GammaTable, &L16);
            Sampled ->Values[i] = (float) wValIn;
    }

    return Sampled;
}






LCMSBOOL _cmsSmoothEndpoints(LPWORD Table, int nEntries)
{
    vec w, y, z;
    int i, Zeros, Poles;



    if (cmsIsLinear(Table, nEntries)) return FALSE; 

    
    if (nEntries > MAX_KNOTS) {
                cmsSignalError(LCMS_ERRC_ABORTED, "_cmsSmoothEndpoints: too many points.");
                return FALSE;
                }

    ZeroMemory(w, nEntries * sizeof(float));
    ZeroMemory(y, nEntries * sizeof(float));
    ZeroMemory(z, nEntries * sizeof(float));

    for (i=0; i < nEntries; i++)
    {
        y[i+1] = (float) Table[i];
        w[i+1] = 1.0;
    }

    w[1]        = 65535.0;
    w[nEntries] = 65535.0;

    smooth2(w, y, z, (float) nEntries, nEntries);

    
    Zeros = Poles = 0;
    for (i=nEntries; i > 1; --i) {

            if (z[i] == 0.) Zeros++;
            if (z[i] >= 65535.) Poles++;
            if (z[i] < z[i-1]) return FALSE; 
    }

    if (Zeros > (nEntries / 3)) return FALSE;  
    if (Poles > (nEntries / 3)) return FALSE;    

    

    for (i=0; i < nEntries; i++) {

        

        float v = z[i+1];

        if (v < 0) v = 0;
        if (v > 65535.) v = 65535.;

        Table[i] = (WORD) floor(v + .5);
        }

    return TRUE;
}
