





















#include "lcms.h"









































int cdecl cmsChooseCnvrt(int Absolute,
                 int Phase1, LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,
                             LPMAT3 ChromaticAdaptationMatrixIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                int DoBlackPointCompensation,
                double AdaptationState,
                 _cmsADJFN *fn1,
                 LPWMAT3 wm, LPWVEC3 wof);






LCMSAPI LPcmsCIEXYZ LCMSEXPORT cmsD50_XYZ(void)
{
    static cmsCIEXYZ D50XYZ = {D50X, D50Y, D50Z};

    return &D50XYZ;
}

LCMSAPI LPcmsCIExyY LCMSEXPORT cmsD50_xyY(void)
{
    static cmsCIExyY D50xyY;
    cmsXYZ2xyY(&D50xyY, cmsD50_XYZ());

    return &D50xyY;
}








#ifdef _MSC_VER
#pragma warning(disable : 4100 4505)
#endif






static
void Rel2RelStepAbsCoefs(double AdaptationState,

                         LPcmsCIEXYZ BlackPointIn,
                         LPcmsCIEXYZ WhitePointIn,
                         LPcmsCIEXYZ IlluminantIn,
                         LPMAT3 ChromaticAdaptationMatrixIn,

                         LPcmsCIEXYZ BlackPointOut,
                         LPcmsCIEXYZ WhitePointOut,
                         LPcmsCIEXYZ IlluminantOut,
                         LPMAT3 ChromaticAdaptationMatrixOut,

                         LPMAT3 m, LPVEC3 of)
{
       
       VEC3 WtPtIn, WtPtInAdapted;
       VEC3 WtPtOut, WtPtOutAdapted;
       MAT3 Scale, m1, m2, m3;
       
       VEC3init(&WtPtIn, WhitePointIn->X, WhitePointIn->Y, WhitePointIn->Z);
       MAT3eval(&WtPtInAdapted, ChromaticAdaptationMatrixIn, &WtPtIn);
                  
       VEC3init(&WtPtOut, WhitePointOut->X, WhitePointOut->Y, WhitePointOut->Z);
       MAT3eval(&WtPtOutAdapted, ChromaticAdaptationMatrixOut, &WtPtOut);

       VEC3init(&Scale.v[0], WtPtInAdapted.n[0] / WtPtOutAdapted.n[0], 0, 0);
       VEC3init(&Scale.v[1], 0, WtPtInAdapted.n[1] / WtPtOutAdapted.n[1], 0);
       VEC3init(&Scale.v[2], 0, 0, WtPtInAdapted.n[2] / WtPtOutAdapted.n[2]);


       

       if (AdaptationState == 1.0) {

           

           CopyMemory(m, &Scale, sizeof(MAT3));

       }
       else {

            
            m1 = *ChromaticAdaptationMatrixIn;
            MAT3inverse(&m1, &m2);
       
            MAT3per(&m3, &m2, &Scale);
            MAT3per(m, &m3, ChromaticAdaptationMatrixOut);
       }

            
       VEC3init(of, 0.0, 0.0, 0.0);
                    
}





static
void ComputeBlackPointCompensationFactors(LPcmsCIEXYZ BlackPointIn,
                      LPcmsCIEXYZ WhitePointIn,
                      LPcmsCIEXYZ IlluminantIn,
                      LPcmsCIEXYZ BlackPointOut,
                      LPcmsCIEXYZ WhitePointOut,
                      LPcmsCIEXYZ IlluminantOut,
                      LPMAT3 m, LPVEC3 of)
{
    

   cmsCIEXYZ RelativeBlackPointIn, RelativeBlackPointOut;
   double ax, ay, az, bx, by, bz, tx, ty, tz;
   
   

   cmsAdaptToIlluminant(&RelativeBlackPointIn,  WhitePointIn, IlluminantIn, BlackPointIn);
   cmsAdaptToIlluminant(&RelativeBlackPointOut, WhitePointOut, IlluminantOut, BlackPointOut);
   
   
   
   
   
   
   
   


   tx = RelativeBlackPointIn.X - IlluminantIn ->X;
   ty = RelativeBlackPointIn.Y - IlluminantIn ->Y;
   tz = RelativeBlackPointIn.Z - IlluminantIn ->Z;

   ax = (RelativeBlackPointOut.X - IlluminantOut ->X) / tx;
   ay = (RelativeBlackPointOut.Y - IlluminantOut ->Y) / ty;
   az = (RelativeBlackPointOut.Z - IlluminantOut ->Z) / tz;

   bx = - IlluminantOut -> X * (RelativeBlackPointOut.X - RelativeBlackPointIn.X) / tx;
   by = - IlluminantOut -> Y * (RelativeBlackPointOut.Y - RelativeBlackPointIn.Y) / ty;
   bz = - IlluminantOut -> Z * (RelativeBlackPointOut.Z - RelativeBlackPointIn.Z) / tz;


   MAT3identity(m);

   m->v[VX].n[0] = ax;
   m->v[VY].n[1] = ay;
   m->v[VZ].n[2] = az;

   VEC3init(of, bx, by, bz);

}



static
LCMSBOOL IdentityParameters(LPWMAT3 m, LPWVEC3 of)
{   
    WVEC3 wv0;

    VEC3initF(&wv0, 0, 0, 0);

    if (!MAT3isIdentity(m, 0.00001)) return FALSE;
    if (!VEC3equal(of, &wv0, 0.00001)) return FALSE;

    return TRUE;
}








static
void XYZ2XYZ(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{

    WVEC3 a, r;

    a.n[0] = In[0] << 1;
    a.n[1] = In[1] << 1;
    a.n[2] = In[2] << 1;

    MAT3evalW(&r, m, &a);

    Out[0] = _cmsClampWord((r.n[VX] + of->n[VX]) >> 1);
    Out[1] = _cmsClampWord((r.n[VY] + of->n[VY]) >> 1);
    Out[2] = _cmsClampWord((r.n[VZ] + of->n[VZ]) >> 1);
}




static
void XYZ2Lab(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{
  WORD XYZ[3];

  XYZ2XYZ(In, XYZ, m, of);
  cmsXYZ2LabEncoded(XYZ, Out);
}



static
void Lab2XYZ(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{
       WORD XYZ[3];

       cmsLab2XYZEncoded(In, XYZ);
       XYZ2XYZ(XYZ, Out, m, of);
}



static
void Lab2XYZ2Lab(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{
       WORD XYZ[3], XYZ2[3];

       cmsLab2XYZEncoded(In, XYZ);
       XYZ2XYZ(XYZ, XYZ2, m, of);
       cmsXYZ2LabEncoded(XYZ2, Out);
}





static
int FromXYZRelLUT(int Absolute,
                             LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,
                             LPMAT3 ChromaticAdaptationMatrixIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                 int DoBlackPointCompensation,
                 double AdaptationState,
                 _cmsADJFN *fn1,
                 LPMAT3 m, LPVEC3 of)

{
              switch (Phase2) {

                     

                     case XYZRel:

                            if (Absolute)
                            {
                                   
                                   

                                   Rel2RelStepAbsCoefs(AdaptationState,
                                                       BlackPointIn,
                                                       WhitePointIn,
                                                       IlluminantIn,
                                                       ChromaticAdaptationMatrixIn,
                                                       BlackPointOut,
                                                       WhitePointOut,
                                                       IlluminantOut,
                                                       ChromaticAdaptationMatrixOut,
                                                       m, of);
                                   *fn1 = XYZ2XYZ;

                            }
                            else
                            {
                                   
                                   *fn1 = NULL;
                                   if (DoBlackPointCompensation) {

                                      *fn1 = XYZ2XYZ;
                                      ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                      WhitePointIn,
                                                                      IlluminantIn,
                                                                      BlackPointOut,
                                                                      WhitePointOut,
                                                                      IlluminantOut,
                                                                      m, of);

                                   }
                            }
                            break;

                    
                     

                     case LabRel:

                            
                            
                            

                            if (Absolute)
                            {   

                                Rel2RelStepAbsCoefs(AdaptationState,
                                                    BlackPointIn,
                                                    WhitePointIn,
                                                    IlluminantIn,
                                                    ChromaticAdaptationMatrixIn,
                                                    BlackPointOut,
                                                    WhitePointOut,
                                                    IlluminantOut,
                                                    ChromaticAdaptationMatrixOut,
                                                    m, of);
                                
                                *fn1 = XYZ2Lab;

                            }
                            else
                            {
                                   

                                   MAT3identity(m);
                                   VEC3init(of, 0, 0, 0);
                                   *fn1 = XYZ2Lab;

                                   if (DoBlackPointCompensation) {

                                    ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                          WhitePointIn,
                                                                          IlluminantIn,
                                                                          BlackPointOut,
                                                                          WhitePointOut,
                                                                          IlluminantOut,
                                                                          m, of);
                                   }
                            }
                            break;

                    
                     default: return FALSE;
                     }

              return TRUE;
}






static
int FromLabRelLUT(int Absolute,
                             LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,
                             LPMAT3 ChromaticAdaptationMatrixIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                int DoBlackPointCompensation,
                double AdaptationState,

                 _cmsADJFN *fn1,
                 LPMAT3 m, LPVEC3 of)
{

          switch (Phase2) {

              

              case XYZRel:

                  if (Absolute) {  

                            
                            

                            Rel2RelStepAbsCoefs(AdaptationState,
                                                BlackPointIn,
                                                WhitePointIn,
                                                cmsD50_XYZ(),
                                                ChromaticAdaptationMatrixIn,
                                                BlackPointOut,
                                                WhitePointOut,
                                                IlluminantOut,
                                                ChromaticAdaptationMatrixOut,
                                                m, of);

                            *fn1 = Lab2XYZ;

                     }
                     else
                     {
                            
                            
                            *fn1 = Lab2XYZ;
                            if (DoBlackPointCompensation) {

                                 ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                      WhitePointIn,
                                                                      IlluminantIn,
                                                                      BlackPointOut,
                                                                      WhitePointOut,
                                                                      IlluminantOut,
                                                                      m, of);

                            }
                     }
                     break;



              case LabRel:

                     if (Absolute) {

                             
                             
                             

                             Rel2RelStepAbsCoefs(AdaptationState,
                                                 BlackPointIn, 
                                                 WhitePointIn, IlluminantIn,
                                                 ChromaticAdaptationMatrixIn,
                                                 BlackPointOut, 
                                                 WhitePointOut, cmsD50_XYZ(),
                                                 ChromaticAdaptationMatrixOut,
                                                 m, of);
                             *fn1 = Lab2XYZ2Lab;
                     }
                     else
                     {      
                            

                            *fn1 = NULL;
                             if (DoBlackPointCompensation) {

                                 *fn1 = Lab2XYZ2Lab;
                                 ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                      WhitePointIn,
                                                                      IlluminantIn,
                                                                      BlackPointOut,
                                                                      WhitePointOut,
                                                                      IlluminantOut,
                                                                      m, of);

                                
                            }
                     }
                     break;


              default: return FALSE;
              }

   return TRUE;
}













int cmsChooseCnvrt(int Absolute,
                  int Phase1, LPcmsCIEXYZ BlackPointIn,
                              LPcmsCIEXYZ WhitePointIn,
                              LPcmsCIEXYZ IlluminantIn,
                              LPMAT3 ChromaticAdaptationMatrixIn,

                  int Phase2, LPcmsCIEXYZ BlackPointOut,
                              LPcmsCIEXYZ WhitePointOut,
                              LPcmsCIEXYZ IlluminantOut,
                              LPMAT3 ChromaticAdaptationMatrixOut,

                  int DoBlackPointCompensation,
                  double AdaptationState,
                  _cmsADJFN *fn1,
                  LPWMAT3 wm, LPWVEC3 wof)
{

       int rc;
       MAT3 m;
       VEC3 of;


       MAT3identity(&m);
       VEC3init(&of, 0, 0, 0);

       switch (Phase1) {

       

       case XYZRel:  rc = FromXYZRelLUT(Absolute,
                                          BlackPointIn,
                                          WhitePointIn,
                                          IlluminantIn,
                                          ChromaticAdaptationMatrixIn,
                                          Phase2,
                                          BlackPointOut,
                                          WhitePointOut,
                                          IlluminantOut,
                                          ChromaticAdaptationMatrixOut,
                                          DoBlackPointCompensation,
                                          AdaptationState,
                                          fn1, &m, &of);
                     break;

       

       

       case LabRel:  rc =  FromLabRelLUT(Absolute,
                                          BlackPointIn,
                                          WhitePointIn,
                                          IlluminantIn,
                                          ChromaticAdaptationMatrixIn,
                                          Phase2,
                                          BlackPointOut,
                                          WhitePointOut,
                                          IlluminantOut,
                                          ChromaticAdaptationMatrixOut,
                                          DoBlackPointCompensation,
                                          AdaptationState,
                                          fn1, &m, &of);
                     break;


       

       

       default:    cmsSignalError(LCMS_ERRC_ABORTED, "(internal) Phase error");
                   return FALSE;

       }

       MAT3toFix(wm, &m);
       VEC3toFix(wof, &of);

       

       if (*fn1 == XYZ2XYZ || *fn1 == Lab2XYZ2Lab) {

           if (IdentityParameters(wm, wof))
               *fn1 = NULL;
       }

       
       return rc;
}


      
