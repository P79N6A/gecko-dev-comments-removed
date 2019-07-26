



#ifndef CKMD_H
#define CKMD_H






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
