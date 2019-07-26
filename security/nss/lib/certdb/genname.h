



#ifndef _GENAME_H_
#define _GENAME_H_

#include "plarena.h"
#include "seccomon.h"
#include "secoidt.h"
#include "secasn1.h"
#include "secder.h"
#include "certt.h"


SEC_BEGIN_PROTOS

extern const SEC_ASN1Template CERT_GeneralNamesTemplate[];

extern SECItem **
cert_EncodeGeneralNames(PRArenaPool *arena, CERTGeneralName *names);

extern CERTGeneralName *
cert_DecodeGeneralNames(PRArenaPool *arena, SECItem **encodedGenName);

extern SECStatus
cert_DestroyGeneralNames(CERTGeneralName *name);

extern SECStatus 
cert_EncodeNameConstraints(CERTNameConstraints *constraints, PRArenaPool *arena,
			   SECItem *dest);

extern CERTNameConstraints *
cert_DecodeNameConstraints(PRArenaPool *arena, SECItem *encodedConstraints);

extern CERTGeneralName *
cert_CombineNamesLists(CERTGeneralName *list1, CERTGeneralName *list2);

extern CERTNameConstraint *
cert_CombineConstraintsLists(CERTNameConstraint *list1, CERTNameConstraint *list2);






void
CERT_DestroyGeneralName(CERTGeneralName *name);

SECStatus
CERT_CompareGeneralName(CERTGeneralName *a, CERTGeneralName *b);

SECStatus
CERT_CopyGeneralName(PRArenaPool      *arena, 
		     CERTGeneralName  *dest, 
		     CERTGeneralName  *src);





void
CERT_DestroyGeneralNameList(CERTGeneralNameList *list);


CERTGeneralNameList *
CERT_CreateGeneralNameList(CERTGeneralName *name);


SECStatus
CERT_CompareGeneralNameLists(CERTGeneralNameList *a, CERTGeneralNameList *b);


void *
CERT_GetGeneralNameFromListByType(CERTGeneralNameList *list,
				  CERTGeneralNameType type,
				  PRArenaPool *arena);


void
CERT_AddGeneralNameToList(CERTGeneralNameList *list, 
			  CERTGeneralNameType type,
			  void *data, SECItem *oid);


CERTGeneralNameList *
CERT_DupGeneralNameList(CERTGeneralNameList *list);




extern int
CERT_GetNamesLength(CERTGeneralName *names);



SECStatus
CERT_CompareNameSpace(CERTCertificate  *cert,
		      CERTGeneralName  *namesList,
 		      CERTCertificate **certsList,
 		      PRArenaPool      *reqArena,
 		      CERTCertificate **pBadCert);

SEC_END_PROTOS

#endif
