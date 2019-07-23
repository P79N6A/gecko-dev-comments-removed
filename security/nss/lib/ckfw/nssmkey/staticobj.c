





































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: staticobj.c,v $ $Revision: 1.2 $ $Date: 2008/10/05 20:59:22 $""; @(#) $RCSfile: staticobj.c,v $ $Revision: 1.2 $ $Date: 2008/10/05 20:59:22 $";
#endif 

#ifndef CKMK_H
#include "ckmk.h"
#endif 

static const CK_TRUST ckt_netscape_valid = CKT_NETSCAPE_VALID;
static const CK_OBJECT_CLASS cko_certificate = CKO_CERTIFICATE;
static const CK_TRUST ckt_netscape_trusted_delegator = CKT_NETSCAPE_TRUSTED_DELEGATOR;
static const CK_OBJECT_CLASS cko_netscape_trust = CKO_NETSCAPE_TRUST;
static const CK_BBOOL ck_true = CK_TRUE;
static const CK_OBJECT_CLASS cko_data = CKO_DATA;
static const CK_CERTIFICATE_TYPE ckc_x_509 = CKC_X_509;
static const CK_BBOOL ck_false = CK_FALSE;
static const CK_OBJECT_CLASS cko_netscape_builtin_root_list = CKO_NETSCAPE_BUILTIN_ROOT_LIST;


static const CK_ATTRIBUTE_TYPE nss_ckmk_types_1 [] = {
 CKA_CLASS,  CKA_TOKEN,  CKA_PRIVATE,  CKA_MODIFIABLE,  CKA_LABEL
};

static const NSSItem nss_ckmk_items_1 [] = {
  { (void *)&cko_data, (PRUint32)sizeof(CK_OBJECT_CLASS) },
  { (void *)&ck_true, (PRUint32)sizeof(CK_BBOOL) },
  { (void *)&ck_false, (PRUint32)sizeof(CK_BBOOL) },
  { (void *)&ck_false, (PRUint32)sizeof(CK_BBOOL) },
  { (void *)"Mozilla Mac Key Ring Access", (PRUint32)28 }
};

ckmkInternalObject nss_ckmk_data[] = {
  { ckmkRaw, {{ 5, nss_ckmk_types_1, nss_ckmk_items_1}} , CKO_DATA, {NULL} },
};

const PRUint32 nss_ckmk_nObjects = 1;
