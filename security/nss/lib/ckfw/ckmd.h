



#ifndef CKMD_H
#define CKMD_H

#ifdef DEBUG
static const char CKMD_CVS_ID[] = "@(#) $RCSfile: ckmd.h,v $ $Revision: 1.4 $ $Date: 2012/04/25 14:49:28 $";
#endif 






NSS_EXTERN NSSCKMDObject *
nssCKMDSessionObject_Create
(
  NSSCKFWToken *fwToken,
  NSSArena *arena,
  CK_ATTRIBUTE_PTR attributes,
  CK_ULONG ulCount,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDFindObjects *
nssCKMDFindSessionObjects_Create
(
  NSSCKFWToken *fwToken,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulCount,
  CK_RV *pError
);

#endif 
