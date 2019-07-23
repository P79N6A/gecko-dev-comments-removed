





















#include "lcms.h"






#define cmsAllocPrecacheTables(aProf, cacheType, unionMemb, nTables, elemSize, nElems) \
{\
       unsigned i, j; \
       for (i = 0; i < nTables; ++i) { \
              aProf->Precache[cacheType]->Impl.unionMemb.Cache[i] = \
                _cmsMalloc(elemSize * nElems); \
              if (aProf->Precache[cacheType]->Impl.unionMemb.Cache[i] == NULL) { \
                     for (j = 0; j < i; ++j) \
                            _cmsFree(aProf->Precache[cacheType]->Impl.unionMemb.Cache[j]); \
                     _cmsFree(aProf->Precache[cacheType]); \
                     aProf->Precache[cacheType] = NULL; \
                     return FALSE; \
              } \
       } \
}








LCMSBOOL LCMSEXPORT cmsPrecacheProfile(cmsHPROFILE hProfile, 
                                       LCMSPRECACHETYPE Type) {

       
       LPGAMMATABLE GTables[3];
       LCMSBOOL hasGammaTables;
       LPLCMSICCPROFILE Icc = (LPLCMSICCPROFILE) (LPSTR) hProfile;
       L16PARAMS p16;
       unsigned i, j;

       
       CMSASSERT(Type < PRECACHE_TYPE_COUNT);

       
       if (Icc->Precache[Type] != NULL)
              return TRUE;

       
       hasGammaTables = cmsIsTag(hProfile, icSigRedTRCTag) &&
                        cmsIsTag(hProfile, icSigGreenTRCTag) &&
                        cmsIsTag(hProfile, icSigBlueTRCTag);

       
       ZeroMemory(GTables, sizeof(GTables));

       
       Icc->Precache[Type] = _cmsMalloc(sizeof(LCMSPRECACHE));
       if (Icc->Precache[Type] == NULL)
              return FALSE;
       ZeroMemory(Icc->Precache[Type], sizeof(LCMSPRECACHE));

       
       PRECACHE_ADDREF(Icc->Precache[Type]);

       
       Icc->Precache[Type]->Type = Type;

       
       if (IS_LI_REVERSE(Type)) {

              
              if (hasGammaTables) {
                     GTables[0] = cmsReadICCGammaReversed(hProfile, icSigRedTRCTag);
                     GTables[1] = cmsReadICCGammaReversed(hProfile, icSigGreenTRCTag);
                     GTables[2] = cmsReadICCGammaReversed(hProfile, icSigBlueTRCTag);
              }

              
              if (!GTables[0] || !GTables[1] || !GTables[2]) {
                     _cmsFree(Icc->Precache[Type]);
                     Icc->Precache[Type] = NULL;
                     return FALSE;
              }
       }
       else if (IS_LI_FORWARD(Type)) {

              
              if (hasGammaTables) {
                     GTables[0] = cmsReadICCGamma(hProfile, icSigRedTRCTag);
                     GTables[1] = cmsReadICCGamma(hProfile, icSigGreenTRCTag);
                     GTables[2] = cmsReadICCGamma(hProfile, icSigBlueTRCTag);
              }

              
              if (!GTables[0] || !GTables[1] || !GTables[2]) {
                     _cmsFree(Icc->Precache[Type]);
                     Icc->Precache[Type] = NULL;
                     return FALSE;
              }
       }

       
       switch(Type) {

              case CMS_PRECACHE_LI1616_REVERSE:

                     
                     cmsAllocPrecacheTables(Icc, Type, LI1616_REVERSE, 3, sizeof(WORD), (1 << 16));

                     
                     cmsCalcL16Params(GTables[0]->nEntries, &p16);

                     
                     for (i = 0; i < 3; ++i)
                            for (j = 0; j < (1 << 16); ++j)
                                   Icc->Precache[Type]->Impl.LI1616_REVERSE.Cache[i][j] =
                                          cmsLinearInterpLUT16((WORD)j, GTables[i]->GammaTable, &p16);
                     break;

              case CMS_PRECACHE_LI16W_FORWARD:

                     
                     cmsAllocPrecacheTables(Icc, Type, LI16W_FORWARD, 3, sizeof(Fixed32), (1 << 16));

                     
                     cmsCalcL16Params(GTables[0]->nEntries, &p16);

                     
                     for (i = 0; i < 3; ++i)
                            for (j = 0; j < (1 << 16); ++j)
                                   Icc->Precache[Type]->Impl.LI16W_FORWARD.Cache[i][j] =
                                          cmsLinearInterpFixed((WORD)j, GTables[i]->GammaTable, &p16);
                     break;

              default:
                     CMSASSERT(0); 
                     break;
       }

       
       return TRUE;
}







void cmsPrecacheFree(LPLCMSPRECACHE Cache) {

       
       unsigned i;

       
       CMSASSERT(Cache != NULL);
       CMSASSERT(Cache->RefCount == 0);

       
       switch(Cache->Type) {

              case CMS_PRECACHE_LI1616_REVERSE:
                     for (i = 0; i < 3; ++i)
                      _cmsFree(Cache->Impl.LI1616_REVERSE.Cache[i]);
                     break;

              case CMS_PRECACHE_LI16W_FORWARD:
                     for (i = 0; i < 3; ++i)
                      _cmsFree(Cache->Impl.LI16W_FORWARD.Cache[i]);
                     break;

              default:
                     CMSASSERT(0); 
                     break;
       }

       
       _cmsFree(Cache);

}
