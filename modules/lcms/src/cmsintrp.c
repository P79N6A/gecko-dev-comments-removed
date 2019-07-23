























#include "lcms.h"

void cmsCalcL16Params(int nSamples, LPL16PARAMS p)
{
       p -> nSamples = nSamples;
       p -> Domain   = (WORD) (nSamples - 1);
       p -> nInputs = p -> nOutputs = 1;
      
}





static
void Eval1Input(WORD StageABC[], WORD StageLMN[], WORD LutTable[], LPL16PARAMS p16)
{
       Fixed32 fk;
       Fixed32 k0, k1, rk, K0, K1;
       int OutChan;

       fk = ToFixedDomain((Fixed32) StageABC[0] * p16 -> Domain);
       k0 = FIXED_TO_INT(fk);
       rk = (WORD) FIXED_REST_TO_INT(fk);

       k1 = k0 + (StageABC[0] != 0xFFFFU ? 1 : 0);

       K0 = p16 -> opta1 * k0;
       K1 = p16 -> opta1 * k1;

       for (OutChan=0; OutChan < p16->nOutputs; OutChan++) {

           StageLMN[OutChan] = (WORD) FixedLERP(rk, LutTable[K0+OutChan],
                                                    LutTable[K1+OutChan]);
       }
}





static
void Eval4Inputs(WORD StageABC[], WORD StageLMN[], WORD LutTable[], LPL16PARAMS p16)
{       
       Fixed32 fk;
       Fixed32 k0, rk;
       int K0, K1;
       LPWORD T;
       int i;
       WORD Tmp1[MAXCHANNELS], Tmp2[MAXCHANNELS];

       
       fk = ToFixedDomain((Fixed32) StageABC[0] * p16 -> Domain);
       k0 = FIXED_TO_INT(fk);
       rk = FIXED_REST_TO_INT(fk);

       K0 = p16 -> opta4 * k0;
       K1 = p16 -> opta4 * (k0 + (StageABC[0] != 0xFFFFU ? 1 : 0));

       p16 -> nInputs = 3;

       T = LutTable + K0;

       cmsTetrahedralInterp16(StageABC + 1,  Tmp1, T, p16);

      
       T = LutTable + K1;

       cmsTetrahedralInterp16(StageABC + 1,  Tmp2, T, p16);

      
       p16 -> nInputs = 4;
       for (i=0; i < p16 -> nOutputs; i++)
       {
              StageLMN[i] = (WORD) FixedLERP(rk, Tmp1[i], Tmp2[i]);
              
       }

}


static
void Eval5Inputs(WORD StageABC[], WORD StageLMN[], WORD LutTable[], LPL16PARAMS p16)
{       
       Fixed32 fk;
       Fixed32 k0, rk;
       int K0, K1;
       LPWORD T;
       int i;
       WORD Tmp1[MAXCHANNELS], Tmp2[MAXCHANNELS];

       
       fk = ToFixedDomain((Fixed32) StageABC[0] * p16 -> Domain);
       k0 = FIXED_TO_INT(fk);
       rk = FIXED_REST_TO_INT(fk);

       K0 = p16 -> opta5 * k0;
       K1 = p16 -> opta5 * (k0 + (StageABC[0] != 0xFFFFU ? 1 : 0));

       p16 -> nInputs = 4;

       T = LutTable + K0;

       Eval4Inputs(StageABC + 1, Tmp1, T, p16);

       T = LutTable + K1;

       Eval4Inputs(StageABC + 1, Tmp2, T, p16);

       p16 -> nInputs = 5;
       for (i=0; i < p16 -> nOutputs; i++)
       {
              StageLMN[i] = (WORD) FixedLERP(rk, Tmp1[i], Tmp2[i]);           
              
       }

}


static
void Eval6Inputs(WORD StageABC[], WORD StageLMN[], WORD LutTable[], LPL16PARAMS p16)
{       
       Fixed32 fk;
       Fixed32 k0, rk;
       int K0, K1;
       LPWORD T;
       int i;
       WORD Tmp1[MAXCHANNELS], Tmp2[MAXCHANNELS];

       
       fk = ToFixedDomain((Fixed32) StageABC[0] * p16 -> Domain);
       k0 = FIXED_TO_INT(fk);
       rk = FIXED_REST_TO_INT(fk);

       K0 = p16 -> opta6 * k0;
       K1 = p16 -> opta6 * (k0 + (StageABC[0] != 0xFFFFU ? 1 : 0));

       p16 -> nInputs = 5;

       T = LutTable + K0;

       Eval5Inputs(StageABC + 1, Tmp1, T, p16);

       T = LutTable + K1;

       Eval5Inputs(StageABC + 1, Tmp2, T, p16);

       p16 -> nInputs = 6;
       for (i=0; i < p16 -> nOutputs; i++)
       {
              StageLMN[i] = (WORD) FixedLERP(rk, Tmp1[i], Tmp2[i]);
       }

}

static
void Eval7Inputs(WORD StageABC[], WORD StageLMN[], WORD LutTable[], LPL16PARAMS p16)
{       
       Fixed32 fk;
       Fixed32 k0, rk;
       int K0, K1;
       LPWORD T;
       int i;
       WORD Tmp1[MAXCHANNELS], Tmp2[MAXCHANNELS];

       
       fk = ToFixedDomain((Fixed32) StageABC[0] * p16 -> Domain);
       k0 = FIXED_TO_INT(fk);
       rk = FIXED_REST_TO_INT(fk);

       K0 = p16 -> opta7 * k0;
       K1 = p16 -> opta7 * (k0 + (StageABC[0] != 0xFFFFU ? 1 : 0));

       p16 -> nInputs = 6;

       T = LutTable + K0;

       Eval6Inputs(StageABC + 1, Tmp1, T, p16);

       T = LutTable + K1;

       Eval6Inputs(StageABC + 1, Tmp2, T, p16);

       p16 -> nInputs = 7;
       for (i=0; i < p16 -> nOutputs; i++)
       {
              StageLMN[i] = (WORD) FixedLERP(rk, Tmp1[i], Tmp2[i]);
       }

}

static
void Eval8Inputs(WORD StageABC[], WORD StageLMN[], WORD LutTable[], LPL16PARAMS p16)
{       
       Fixed32 fk;
       Fixed32 k0, rk;
       int K0, K1;
       LPWORD T;
       int i;
       WORD Tmp1[MAXCHANNELS], Tmp2[MAXCHANNELS];

       
       fk = ToFixedDomain((Fixed32) StageABC[0] * p16 -> Domain);
       k0 = FIXED_TO_INT(fk);
       rk = FIXED_REST_TO_INT(fk);

       K0 = p16 -> opta8 * k0;
       K1 = p16 -> opta8 * (k0 + (StageABC[0] != 0xFFFFU ? 1 : 0));

       p16 -> nInputs = 7;

       T = LutTable + K0;

       Eval7Inputs(StageABC + 1, Tmp1, T, p16);

       T = LutTable + K1;

       Eval7Inputs(StageABC + 1, Tmp2, T, p16);

       p16 -> nInputs = 8;
       for (i=0; i < p16 -> nOutputs; i++)
       {
              StageLMN[i] = (WORD) FixedLERP(rk, Tmp1[i], Tmp2[i]);
       }

}




void cmsCalcCLUT16ParamsEx(int nSamples, int InputChan, int OutputChan, 
                                            LCMSBOOL lUseTetrahedral, LPL16PARAMS p)
{
       int clutPoints;

       cmsCalcL16Params(nSamples, p);

       p -> nInputs  = InputChan;
       p -> nOutputs = OutputChan;

       clutPoints = p -> Domain + 1;

       p -> opta1 = p -> nOutputs;              
       p -> opta2 = p -> opta1 * clutPoints;    
       p -> opta3 = p -> opta2 * clutPoints;    
       p -> opta4 = p -> opta3 * clutPoints;    
       p -> opta5 = p -> opta4 * clutPoints;    
       p -> opta6 = p -> opta5 * clutPoints;    
       p -> opta7 = p -> opta6 * clutPoints;    
       p -> opta8 = p -> opta7 * clutPoints;    


       switch (InputChan) {


           case 1: 

               p ->Interp3D = Eval1Input;
               break;

           case 3:  
               if (lUseTetrahedral) {                   
                   p ->Interp3D = cmsTetrahedralInterp16;                   
               }
               else
                   p ->Interp3D = cmsTrilinearInterp16;   
               break;

           case 4:  
                p ->Interp3D = Eval4Inputs;
                break;

           case 5: 
                p ->Interp3D = Eval5Inputs;
                break;           

           case 6: 
                p -> Interp3D = Eval6Inputs;
                break;     
                
            case 7: 
                p ->Interp3D = Eval7Inputs;
                break;

           case 8: 
                p ->Interp3D = Eval8Inputs;
                break;

           default:
                cmsSignalError(LCMS_ERRC_ABORTED, "Unsupported restoration (%d channels)", InputChan);
           }
       
}


void cmsCalcCLUT16Params(int nSamples, int InputChan, int OutputChan, LPL16PARAMS p)
{
    cmsCalcCLUT16ParamsEx(nSamples, InputChan, OutputChan, FALSE, p);
}



#ifdef USE_FLOAT




WORD cmsLinearInterpLUT16(WORD Value, WORD LutTable[], LPL16PARAMS p)
{
       double y1, y0;
       double y;
       double val2, rest;
       int cell0, cell1;

       

       if (Value == 0xffff) return LutTable[p -> Domain];

       val2 = p -> Domain * ((double) Value / 65535.0);

       cell0 = (int) floor(val2);
       cell1 = (int) ceil(val2);

       

       rest = val2 - cell0;

       y0 = LutTable[cell0] ;
       y1 = LutTable[cell1] ;

       y = y0 + (y1 - y0) * rest;


       return (WORD) floor(y+.5);
}

#endif







#ifdef USE_C

WORD cmsLinearInterpLUT16(WORD Value1, WORD LutTable[], LPL16PARAMS p)
{
       WORD y1, y0;
       WORD y;
       int dif, a1;
       int cell0, rest;
       int val3, Value;

       


       Value = Value1;
       if (Value == 0xffff) return LutTable[p -> Domain];

       val3 = p -> Domain * Value;
       val3 = ToFixedDomain(val3);              

       cell0 = FIXED_TO_INT(val3);             
       rest  = FIXED_REST_TO_INT(val3);        

       y0 = LutTable[cell0] ;
       y1 = LutTable[cell0+1] ;

       dif = (int) y1 - y0;        

       if (dif >= 0)
       {
       a1 = ToFixedDomain(dif * rest);
       a1 += 0x8000;
       }
       else
       {
              a1 = ToFixedDomain((- dif) * rest);
              a1 -= 0x8000;
              a1 = -a1;
       }

       y = (WORD) (y0 + FIXED_TO_INT(a1));

       return y;
}

#endif



#ifdef USE_ASSEMBLER

#ifdef _MSC_VER
#pragma warning(disable : 4033)
#pragma warning(disable : 4035)
#endif

WORD cmsLinearInterpLUT16(WORD Value, WORD LutTable[], LPL16PARAMS p)
{
       int xDomain = p -> Domain;


       if (Value == 0xffff) return LutTable[p -> Domain];
       else
       ASM {
              xor       eax, eax
              mov       ax, word ptr ss:Value
              mov       edx, ss:xDomain
              mul       edx                         
              shld      edx, eax, 16                
              shl       eax, 16                     
              mov       ebx, 0x0000ffff
              div       ebx
              mov       ecx, eax
              sar       ecx, 16                        
              mov       edx, eax                       
              and       edx, 0x0000ffff                
              mov       ebx, ss:LutTable
              lea       eax, dword ptr [ebx+2*ecx]     
              xor       ebx, ebx
              mov        bx, word  ptr [eax]           
              movzx     eax, word  ptr [eax+2]         
              sub       eax, ebx                       
              js        IsNegative
              mul       edx                            
              shld      edx, eax, 16                   
              sal       eax, 16                        
              mov       ecx, 0x0000ffff
              div       ecx
              add       eax, 0x8000                    
              sar       eax, 16
              add       eax, ebx                       
              }

              RET((WORD) _EAX);

       IsNegative:

              ASM {
              neg       eax
              mul       edx                            
              shld      edx, eax, 16                   
              sal       eax, 16                        
              mov       ecx, 0x0000ffff
              div       ecx
              sub       eax, 0x8000
              neg       eax
              sar       eax, 16
              add       eax, ebx                       
              }

              RET((WORD) _EAX);
}

#ifdef _MSC_VER
#pragma warning(default : 4033)
#pragma warning(default : 4035)
#endif

#endif

Fixed32 cmsLinearInterpFixed(WORD Value1, WORD LutTable[], LPL16PARAMS p)
{
       Fixed32 y1, y0;
       int cell0;
       int val3, Value;

       


       Value = Value1;
       if (Value == 0xffffU) return LutTable[p -> Domain];

       val3 = p -> Domain * Value;
       val3 = ToFixedDomain(val3);              

       cell0 = FIXED_TO_INT(val3);             

       y0 = LutTable[cell0] ;
       y1 = LutTable[cell0+1] ;


       return y0 + FixedMul((y1 - y0), (val3 & 0xFFFFL));
}





WORD cmsReverseLinearInterpLUT16(WORD Value, WORD LutTable[], LPL16PARAMS p)
{
        register int l = 1;
        register int r = 0x10000;
        register int x = 0, res;       
        int NumZeroes, NumPoles;
        int cell0, cell1;
        double val2;
        double y0, y1, x0, x1;
        double a, b, f;

        
        
        
        

        NumZeroes = 0;
        while (LutTable[NumZeroes] == 0 && NumZeroes < p -> Domain)
                        NumZeroes++;

        
        

        if (NumZeroes == 0 && Value == 0)
            return 0;

        NumPoles = 0;
        while (LutTable[p -> Domain - NumPoles] == 0xFFFF && NumPoles < p -> Domain)
                        NumPoles++;

        
        if (NumZeroes > 1 || NumPoles > 1)
        {               
                int a, b;

                
                if (Value == 0) return 0;
               

                

                a = ((NumZeroes-1) * 0xFFFF) / p->Domain;               
                b = ((p -> Domain - NumPoles) * 0xFFFF) / p ->Domain;
                                                                
                l = a - 1;
                r = b + 1;
        }


        

        while (r > l) {

                x = (l + r) / 2;

                res = (int) cmsLinearInterpLUT16((WORD) (x - 1), LutTable, p);

                if (res == Value) {

                    
                    
                    return (WORD) (x - 1);
                }

                if (res > Value) r = x - 1;
                else l = x + 1;
        }

        

                
        
        
        val2 = p -> Domain * ((double) (x - 1) / 65535.0);

        cell0 = (int) floor(val2);
        cell1 = (int) ceil(val2);
           
        if (cell0 == cell1) return (WORD) x;

        y0 = LutTable[cell0] ;
        x0 = (65535.0 * cell0) / p ->Domain; 

        y1 = LutTable[cell1] ;
        x1 = (65535.0 * cell1) / p ->Domain;

        a = (y1 - y0) / (x1 - x0);
        b = y0 - a * x0;

        if (fabs(a) < 0.01) return (WORD) x;

        f = ((Value - b) / a);

        if (f < 0.0) return (WORD) 0;
        if (f >= 65535.0) return (WORD) 0xFFFF;

        return (WORD) floor(f + 0.5);                        
        
}






#ifdef USE_FLOAT
void cmsTrilinearInterp16(WORD Input[], WORD Output[],
                            WORD LutTable[], LPL16PARAMS p)

{
#   define LERP(a,l,h)  (double) ((l)+(((h)-(l))*(a)))
#   define DENS(X, Y, Z)    (double) (LutTable[TotalOut*((Z)+clutPoints*((Y)+clutPoints*(X)))+OutChan])



    double     px, py, pz;
    int        x0, y0, z0,
               x1, y1, z1;
               int clutPoints, TotalOut, OutChan;
    double     fx, fy, fz,
               d000, d001, d010, d011,
               d100, d101, d110, d111,
               dx00, dx01, dx10, dx11,
               dxy0, dxy1, dxyz;


    clutPoints = p -> Domain + 1;
    TotalOut   = p -> nOutputs;

    px = ((double) Input[0] * (p->Domain)) / 65535.0;
    py = ((double) Input[1] * (p->Domain)) / 65535.0;
    pz = ((double) Input[2] * (p->Domain)) / 65535.0;

    x0 = (int) _cmsQuickFloor(px); fx = px - (double) x0;
    y0 = (int) _cmsQuickFloor(py); fy = py - (double) y0;
    z0 = (int) _cmsQuickFloor(pz); fz = pz - (double) z0;

    x1 = x0 + (Input[0] != 0xFFFFU ? 1 : 0);
    y1 = y0 + (Input[1] != 0xFFFFU ? 1 : 0);
    z1 = z0 + (Input[2] != 0xFFFFU ? 1 : 0);


    for (OutChan = 0; OutChan < TotalOut; OutChan++)
    {

        d000 = DENS(x0, y0, z0);
        d001 = DENS(x0, y0, z1);
        d010 = DENS(x0, y1, z0);
        d011 = DENS(x0, y1, z1);

        d100 = DENS(x1, y0, z0);
        d101 = DENS(x1, y0, z1);
        d110 = DENS(x1, y1, z0);
        d111 = DENS(x1, y1, z1);


    dx00 = LERP(fx, d000, d100);
    dx01 = LERP(fx, d001, d101);
    dx10 = LERP(fx, d010, d110);
    dx11 = LERP(fx, d011, d111);

    dxy0 = LERP(fy, dx00, dx10);
    dxy1 = LERP(fy, dx01, dx11);

    dxyz = LERP(fz, dxy0, dxy1);

    Output[OutChan] = (WORD) floor(dxyz + .5);
    }


#   undef LERP
#   undef DENS
}


#endif


#ifndef USE_FLOAT



void cmsTrilinearInterp16(WORD Input[], WORD Output[],
                            WORD LutTable[], LPL16PARAMS p)

{
#define DENS(i,j,k) (LutTable[(i)+(j)+(k)+OutChan])
#define LERP(a,l,h)     (WORD) (l+ ROUND_FIXED_TO_INT(((h-l)*a)))


           int        OutChan, TotalOut;
           Fixed32    fx, fy, fz;
  register int        rx, ry, rz;
           int        x0, y0, z0;
  register int        X0, X1, Y0, Y1, Z0, Z1;
           int        d000, d001, d010, d011,
                      d100, d101, d110, d111,
                      dx00, dx01, dx10, dx11,
                      dxy0, dxy1, dxyz;


    TotalOut   = p -> nOutputs;

    fx = ToFixedDomain((int) Input[0] * p -> Domain);
    x0  = FIXED_TO_INT(fx);
    rx  = FIXED_REST_TO_INT(fx);    


    fy = ToFixedDomain((int) Input[1] * p -> Domain);
    y0  = FIXED_TO_INT(fy);
    ry  = FIXED_REST_TO_INT(fy);

    fz = ToFixedDomain((int) Input[2] * p -> Domain);
    z0 = FIXED_TO_INT(fz);
    rz = FIXED_REST_TO_INT(fz);



    X0 = p -> opta3 * x0;
    X1 = X0 + (Input[0] == 0xFFFFU ? 0 : p->opta3);

	Y0 = p -> opta2 * y0;
    Y1 = Y0 + (Input[1] == 0xFFFFU ? 0 : p->opta2);
   
    Z0 = p -> opta1 * z0;
    Z1 = Z0 + (Input[2] == 0xFFFFU ? 0 : p->opta1);
    


    for (OutChan = 0; OutChan < TotalOut; OutChan++)
    {

        d000 = DENS(X0, Y0, Z0);
        d001 = DENS(X0, Y0, Z1);
        d010 = DENS(X0, Y1, Z0);
        d011 = DENS(X0, Y1, Z1);

        d100 = DENS(X1, Y0, Z0);
        d101 = DENS(X1, Y0, Z1);
        d110 = DENS(X1, Y1, Z0);
        d111 = DENS(X1, Y1, Z1);


        dx00 = LERP(rx, d000, d100);
        dx01 = LERP(rx, d001, d101);
        dx10 = LERP(rx, d010, d110);
        dx11 = LERP(rx, d011, d111);

        dxy0 = LERP(ry, dx00, dx10);
        dxy1 = LERP(ry, dx01, dx11);

        dxyz = LERP(rz, dxy0, dxy1);

        Output[OutChan] = (WORD) dxyz;
    }


#   undef LERP
#   undef DENS
}

#endif


#ifdef USE_FLOAT

#define DENS(X, Y, Z)    (double) (LutTable[TotalOut*((Z)+clutPoints*((Y)+clutPoints*(X)))+OutChan])




void cmsTetrahedralInterp16(WORD Input[],
                            WORD Output[],
                            WORD LutTable[],
                            LPL16PARAMS p)
{
    double     px, py, pz;
    int        x0, y0, z0,
               x1, y1, z1;
    double     fx, fy, fz;
    double     c1=0, c2=0, c3=0;
    int        clutPoints, OutChan, TotalOut;


    clutPoints = p -> Domain + 1;
    TotalOut   = p -> nOutputs;


    px = ((double) Input[0] * p->Domain) / 65535.0;
    py = ((double) Input[1] * p->Domain) / 65535.0;
    pz = ((double) Input[2] * p->Domain) / 65535.0;

    x0 = (int) _cmsQuickFloor(px); fx = (px - (double) x0);
    y0 = (int) _cmsQuickFloor(py); fy = (py - (double) y0);
    z0 = (int) _cmsQuickFloor(pz); fz = (pz - (double) z0);


    x1 = x0 + (Input[0] != 0xFFFFU ? 1 : 0);
    y1 = y0 + (Input[1] != 0xFFFFU ? 1 : 0);
    z1 = z0 + (Input[2] != 0xFFFFU ? 1 : 0);


    for (OutChan=0; OutChan < TotalOut; OutChan++)
    {

       

       if (fx >= fy && fy >= fz)
       {
              c1 = DENS(x1, y0, z0) - DENS(x0, y0, z0);
              c2 = DENS(x1, y1, z0) - DENS(x1, y0, z0);
              c3 = DENS(x1, y1, z1) - DENS(x1, y1, z0);
       }
       else
       if (fx >= fz && fz >= fy)
       {
              c1 = DENS(x1, y0, z0) - DENS(x0, y0, z0);
              c2 = DENS(x1, y1, z1) - DENS(x1, y0, z1);
              c3 = DENS(x1, y0, z1) - DENS(x1, y0, z0);
       }
       else
       if (fz >= fx && fx >= fy)
       {
              c1 = DENS(x1, y0, z1) - DENS(x0, y0, z1);
              c2 = DENS(x1, y1, z1) - DENS(x1, y0, z1);
              c3 = DENS(x0, y0, z1) - DENS(x0, y0, z0);
       }
       else
       if (fy >= fx && fx >= fz)
       {
              c1 = DENS(x1, y1, z0) - DENS(x0, y1, z0);
              c2 = DENS(x0, y1, z0) - DENS(x0, y0, z0);
              c3 = DENS(x1, y1, z1) - DENS(x1, y1, z0);

       }
       else
       if (fy >= fz && fz >= fx)
       {
              c1 = DENS(x1, y1, z1) - DENS(x0, y1, z1);
              c2 = DENS(x0, y1, z0) - DENS(x0, y0, z0);
              c3 = DENS(x0, y1, z1) - DENS(x0, y1, z0);
       }
       else
       if (fz >= fy && fy >= fx)
       {
              c1 = DENS(x1, y1, z1) - DENS(x0, y1, z1);
              c2 = DENS(x0, y1, z1) - DENS(x0, y0, z1);
              c3 = DENS(x0, y0, z1) - DENS(x0, y0, z0);
       }
       else
       { 
         c1 = c2 = c3 = 0;
       
       }


       Output[OutChan] = (WORD) floor((double) DENS(x0,y0,z0) + c1 * fx + c2 * fy + c3 * fz + .5);
       }

}

#undef DENS

#else

#define DENS(i,j,k) (LutTable[(i)+(j)+(k)+OutChan])


void cmsTetrahedralInterp16(WORD Input[],
                            WORD Output[],
                            WORD LutTable1[],
                            LPL16PARAMS p)
{

       Fixed32    fx, fy, fz;
       Fixed32    rx, ry, rz;
       int        x0, y0, z0;
       Fixed32    c0, c1, c2, c3, Rest;       
       int        OutChan;
       Fixed32    X0, X1, Y0, Y1, Z0, Z1;
       int        TotalOut = p -> nOutputs;
       register   LPWORD LutTable = LutTable1;

       

    fx  = ToFixedDomain((int) Input[0] * p -> Domain);
    fy  = ToFixedDomain((int) Input[1] * p -> Domain);
    fz  = ToFixedDomain((int) Input[2] * p -> Domain);

    x0  = FIXED_TO_INT(fx);
    y0  = FIXED_TO_INT(fy); 
    z0  = FIXED_TO_INT(fz);

    rx  = FIXED_REST_TO_INT(fx);   
    ry  = FIXED_REST_TO_INT(fy);      
    rz  = FIXED_REST_TO_INT(fz);

    X0 = p -> opta3 * x0;
    X1 = X0 + (Input[0] == 0xFFFFU ? 0 : p->opta3);

	Y0 = p -> opta2 * y0;
    Y1 = Y0 + (Input[1] == 0xFFFFU ? 0 : p->opta2);
   
    Z0 = p -> opta1 * z0;
    Z1 = Z0 + (Input[2] == 0xFFFFU ? 0 : p->opta1);
    
    

    
    for (OutChan=0; OutChan < TotalOut; OutChan++) {
       
       c0 = DENS(X0, Y0, Z0);

       if (rx >= ry && ry >= rz) {
             
              c1 = DENS(X1, Y0, Z0) - c0;
              c2 = DENS(X1, Y1, Z0) - DENS(X1, Y0, Z0);
              c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
                            
       }
       else
       if (rx >= rz && rz >= ry) {            

              c1 = DENS(X1, Y0, Z0) - c0;
              c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
              c3 = DENS(X1, Y0, Z1) - DENS(X1, Y0, Z0);
                          
       }
       else
       if (rz >= rx && rx >= ry) {
             
              c1 = DENS(X1, Y0, Z1) - DENS(X0, Y0, Z1);
              c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
              c3 = DENS(X0, Y0, Z1) - c0;                            

       }
       else
       if (ry >= rx && rx >= rz) {
              
              c1 = DENS(X1, Y1, Z0) - DENS(X0, Y1, Z0);
              c2 = DENS(X0, Y1, Z0) - c0;
              c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
                            
       }
       else
       if (ry >= rz && rz >= rx) {
             
              c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
              c2 = DENS(X0, Y1, Z0) - c0;
              c3 = DENS(X0, Y1, Z1) - DENS(X0, Y1, Z0);
                           
       }
       else
       if (rz >= ry && ry >= rx) {             

              c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
              c2 = DENS(X0, Y1, Z1) - DENS(X0, Y0, Z1);
              c3 = DENS(X0, Y0, Z1) - c0;
                           
       }
       else  {
              c1 = c2 = c3 = 0;
              
       }
        
        Rest = c1 * rx + c2 * ry + c3 * rz;                
      
		
		
		

		Output[OutChan] = (WORD) (c0 + ((Rest + 0x7FFF) / 0xFFFF));

    }

}



#undef DENS

#endif




#define DENS(i,j,k) (LutTable[(i)+(j)+(k)+OutChan])

void cmsTetrahedralInterp8(WORD Input[],
                           WORD Output[],
                           WORD LutTable[],
                           LPL16PARAMS p)
{

       int        r, g, b;
       Fixed32    rx, ry, rz;            
       Fixed32    c1, c2, c3, Rest;       
       int        OutChan;
       register   Fixed32    X0, X1, Y0, Y1, Z0, Z1;
       int        TotalOut = p -> nOutputs;
       register   LPL8PARAMS p8 = p ->p8; 

    
       
    r = Input[0] >> 8;
    g = Input[1] >> 8;
    b = Input[2] >> 8;

    X0 = X1 = p8->X0[r];
    Y0 = Y1 = p8->Y0[g];
    Z0 = Z1 = p8->Z0[b];

    X1 += (r == 255) ? 0 : p ->opta3;
    Y1 += (g == 255) ? 0 : p ->opta2;
    Z1 += (b == 255) ? 0 : p ->opta1;

    rx = p8 ->rx[r];
    ry = p8 ->ry[g];
    rz = p8 ->rz[b];

    
    
    for (OutChan=0; OutChan < TotalOut; OutChan++) {
              
       if (rx >= ry && ry >= rz)
       {
             
              c1 = DENS(X1, Y0, Z0) - DENS(X0, Y0, Z0);
              c2 = DENS(X1, Y1, Z0) - DENS(X1, Y0, Z0);
              c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
                            
       }
       else
       if (rx >= rz && rz >= ry)
       {            
              c1 = DENS(X1, Y0, Z0) - DENS(X0, Y0, Z0);
              c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
              c3 = DENS(X1, Y0, Z1) - DENS(X1, Y0, Z0);
                          
       }
       else
       if (rz >= rx && rx >= ry)
       {
             
              c1 = DENS(X1, Y0, Z1) - DENS(X0, Y0, Z1);
              c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
              c3 = DENS(X0, Y0, Z1) - DENS(X0, Y0, Z0);                            

       }
       else
       if (ry >= rx && rx >= rz)
       {
              
              c1 = DENS(X1, Y1, Z0) - DENS(X0, Y1, Z0);
              c2 = DENS(X0, Y1, Z0) - DENS(X0, Y0, Z0);
              c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
                            
       }
       else
       if (ry >= rz && rz >= rx)
       {
             
              c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
              c2 = DENS(X0, Y1, Z0) - DENS(X0, Y0, Z0);
              c3 = DENS(X0, Y1, Z1) - DENS(X0, Y1, Z0);
                           
       }
       else
       if (rz >= ry && ry >= rx)
       {             
              c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
              c2 = DENS(X0, Y1, Z1) - DENS(X0, Y0, Z1);
              c3 = DENS(X0, Y0, Z1) - DENS(X0, Y0, Z0);
                           
       }
       else  {
              c1 = c2 = c3 = 0;
              
       }
        

        Rest = c1 * rx + c2 * ry + c3 * rz;
                
        Output[OutChan] = (WORD) (DENS(X0,Y0,Z0) + ((Rest + 0x7FFF) / 0xFFFF));
    }

}

#undef DENS

