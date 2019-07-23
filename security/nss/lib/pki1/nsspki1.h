



































#ifndef NSSPKI1_H
#define NSSPKI1_H

#ifdef DEBUG
static const char NSSPKI1_CVS_ID[] = "@(#) $RCSfile: nsspki1.h,v $ $Revision: 1.3 $ $Date: 2005/01/20 02:25:49 $";
#endif 








#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef NSSPKI1T_H
#include "nsspki1t.h"
#endif 

#ifndef OIDDATA_H
#include "oiddata.h"
#endif 

PR_BEGIN_EXTERN_C














extern const NSSOID *NSS_OID_UNKNOWN;

















NSS_EXTERN NSSOID *
NSSOID_CreateFromBER
(
  NSSBER *berOid
);

extern const NSSError NSS_ERROR_INVALID_BER;
extern const NSSError NSS_ERROR_NO_MEMORY;


















NSS_EXTERN NSSOID *
NSSOID_CreateFromUTF8
(
  NSSUTF8 *stringOid
);

extern const NSSError NSS_ERROR_INVALID_STRING;
extern const NSSError NSS_ERROR_NO_MEMORY;



















NSS_EXTERN NSSDER *
NSSOID_GetDEREncoding
(
  const NSSOID *oid,
  NSSDER *rvOpt,
  NSSArena *arenaOpt
);

extern const NSSError NSS_ERROR_INVALID_NSSOID;
extern const NSSError NSS_ERROR_NO_MEMORY;





















NSS_EXTERN NSSUTF8 *
NSSOID_GetUTF8Encoding
(
  const NSSOID *oid,
  NSSArena *arenaOpt
);

extern const NSSError NSS_ERROR_INVALID_NSSOID;
extern const NSSError NSS_ERROR_NO_MEMORY;





































NSS_EXTERN NSSATAV *
NSSATAV_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *derATAV
);

extern const NSSError NSS_ERROR_INVALID_BER;
extern const NSSError NSS_ERROR_NO_MEMORY;





















NSS_EXTERN NSSATAV *
NSSATAV_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringATAV
);

extern const NSSError NSS_ERROR_UNKNOWN_ATTRIBUTE;
extern const NSSError NSS_ERROR_INVALID_STRING;
extern const NSSError NSS_ERROR_NO_MEMORY;























NSS_EXTERN NSSATAV *
NSSATAV_Create
(
  NSSArena *arenaOpt,
  const NSSOID *oid,
  const void *data,
  PRUint32 length
);

extern const NSSError NSS_ERROR_INVALID_ARENA;
extern const NSSError NSS_ERROR_INVALID_NSSOID;
extern const NSSError NSS_ERROR_INVALID_POINTER;
extern const NSSError NSS_ERROR_NO_MEMORY;



















NSS_EXTERN PRStatus
NSSATAV_Destroy
(
  NSSATAV *atav
);

extern const NSSError NSS_ERROR_INVALID_ATAV;



















NSS_EXTERN NSSDER *
NSSATAV_GetDEREncoding
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);

extern const NSSError NSS_ERROR_INVALID_ATAV;
extern const NSSError NSS_ERROR_NO_MEMORY;





















NSS_EXTERN NSSUTF8 *
NSSATAV_GetUTF8Encoding
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);

extern const NSSError NSS_ERROR_INVALID_ATAV;
extern const NSSError NSS_ERROR_NO_MEMORY;
















NSS_EXTERN const NSSOID *
NSSATAV_GetType
(
  NSSATAV *atav
);

extern const NSSError NSS_ERROR_INVALID_ATAV;



















NSS_EXTERN NSSUTF8 *
NSSATAV_GetValue
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);

extern const NSSError NSS_ERROR_INVALID_ATAV;
extern const NSSError NSS_ERROR_NO_MEMORY;





















NSS_EXTERN PRStatus
NSSATAV_Compare
(
  NSSATAV *atav1,
  NSSATAV *atav2,
  PRBool *equalp
);

extern const NSSError NSS_ERROR_INVALID_ATAV;
extern const NSSError NSS_ERROR_INVALID_ARGUMENT;



















NSS_EXTERN NSSATAV *
NSSATAV_Duplicate
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);

extern const NSSError NSS_ERROR_INVALID_ATAV;
extern const NSSError NSS_ERROR_NO_MEMORY;








































NSS_EXTERN NSSRDN *
NSSRDN_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berRDN
);






















NSS_EXTERN NSSRDN *
NSSRDN_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringRDN
);




















NSS_EXTERN NSSRDN *
NSSRDN_Create
(
  NSSArena *arenaOpt,
  NSSATAV *atav1,
  ...
);



















NSS_EXTERN NSSRDN *
NSSRDN_CreateSimple
(
  NSSArena *arenaOpt,
  NSSATAV *atav
);



















NSS_EXTERN PRStatus
NSSRDN_Destroy
(
  NSSRDN *rdn
);



















NSS_EXTERN NSSDER *
NSSRDN_GetDEREncoding
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);





















NSS_EXTERN NSSUTF8 *
NSSRDN_GetUTF8Encoding
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);























NSS_EXTERN PRStatus
NSSRDN_AddATAV
(
  NSSRDN *rdn,
  NSSATAV *atav
);
















NSS_EXTERN PRUint32
NSSRDN_GetATAVCount
(
  NSSRDN *rdn
);



























NSS_EXTERN NSSATAV *
NSSRDN_GetATAV
(
  NSSRDN *rdn,
  NSSArena *arenaOpt,
  PRUint32 i
);























NSS_EXTERN NSSATAV *
NSSRDN_GetSimpleATAV
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);





















NSS_EXTERN PRStatus
NSSRDN_Compare
(
  NSSRDN *rdn1,
  NSSRDN *rdn2,
  PRBool *equalp
);



















NSS_EXTERN NSSRDN *
NSSRDN_Duplicate
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);






































NSS_EXTERN NSSRDNSeq *
NSSRDNSeq_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berRDNSeq
);






















NSS_EXTERN NSSRDNSeq *
NSSRDNSeq_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringRDNSeq
);




















NSS_EXTERN NSSRDNSeq *
NSSRDNSeq_Create
(
  NSSArena *arenaOpt,
  NSSRDN *rdn1,
  ...
);



















NSS_EXTERN PRStatus
NSSRDNSeq_Destroy
(
  NSSRDNSeq *rdnseq
);



















NSS_EXTERN NSSDER *
NSSRDNSeq_GetDEREncoding
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt
);




















NSS_EXTERN NSSUTF8 *
NSSRDNSeq_GetUTF8Encoding
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt
);






















NSS_EXTERN PRStatus
NSSRDNSeq_AppendRDN
(
  NSSRDNSeq *rdnseq,
  NSSRDN *rdn
);
















NSS_EXTERN PRUint32
NSSRDNSeq_GetRDNCount
(
  NSSRDNSeq *rdnseq
);

























NSS_EXTERN NSSRDN *
NSSRDNSeq_GetRDN
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt,
  PRUint32 i
);





















NSS_EXTERN PRStatus
NSSRDNSeq_Compare
(
  NSSRDNSeq *rdnseq1,
  NSSRDNSeq *rdnseq2,
  PRBool *equalp
);



















NSS_EXTERN NSSRDNSeq *
NSSRDNSeq_Duplicate
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt
);

















































NSS_EXTERN NSSName *
NSSName_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berName
);





















NSS_EXTERN NSSName *
NSSName_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringName
);




























NSS_EXTERN NSSName *
NSSName_Create
(
  NSSArena *arenaOpt,
  NSSNameChoice choice,
  void *arg
);



















NSS_EXTERN PRStatus
NSSName_Destroy
(
  NSSName *name
);



















NSS_EXTERN NSSDER *
NSSName_GetDEREncoding
(
  NSSName *name,
  NSSArena *arenaOpt
);




















NSS_EXTERN NSSUTF8 *
NSSName_GetUTF8Encoding
(
  NSSName *name,
  NSSArena *arenaOpt
);

















NSS_EXTERN NSSNameChoice
NSSName_GetChoice
(
  NSSName *name
);























NSS_EXTERN NSSRDNSeq *
NSSName_GetRDNSequence
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN void *
NSSName_GetSpecifiedChoice
(
  NSSName *name,
  NSSNameChoice choice,
  NSSArena *arenaOpt
);




















NSS_EXTERN PRStatus
NSSName_Compare
(
  NSSName *name1,
  NSSName *name2,
  PRBool *equalp
);



















NSS_EXTERN NSSName *
NSSName_Duplicate
(
  NSSName *name,
  NSSArena *arenaOpt
);


























NSS_EXTERN NSSUTF8 * 
NSSName_GetUID
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSName_GetEmail
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
NSSName_GetCommonName
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
NSSName_GetOrganization
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 ** 
NSSName_GetOrganizationalUnits
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
NSSName_GetStateOrProvince
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
NSSName_GetLocality
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSName_GetCountry
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSName_GetAttribute
(
  NSSName *name,
  NSSOID *attribute,
  NSSArena *arenaOpt
);
























































NSS_EXTERN NSSGeneralName *
NSSGeneralName_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berGeneralName
);




















NSS_EXTERN NSSGeneralName *
NSSGeneralName_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringGeneralName
);




































NSS_EXTERN NSSGeneralName *
NSSGeneralName_Create
(
  NSSGeneralNameChoice choice,
  void *arg
);



















NSS_EXTERN PRStatus
NSSGeneralName_Destroy
(
  NSSGeneralName *generalName
);



















NSS_EXTERN NSSDER *
NSSGeneralName_GetDEREncoding
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);





















NSS_EXTERN NSSUTF8 *
NSSGeneralName_GetUTF8Encoding
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);


















NSS_EXTERN NSSGeneralNameChoice
NSSGeneralName_GetChoice
(
  NSSGeneralName *generalName
);























NSS_EXTERN NSSOtherName *
NSSGeneralName_GetOtherName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSRFC822Name *
NSSGeneralName_GetRfc822Name
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSDNSName *
NSSGeneralName_GetDNSName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSX400Address *
NSSGeneralName_GetX400Address
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSName *
NSSGeneralName_GetName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSEdiPartyName *
NSSGeneralName_GetEdiPartyName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSURI *
NSSGeneralName_GetUniformResourceIdentifier
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSIPAddress *
NSSGeneralName_GetIPAddress
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSRegisteredID *
NSSGeneralName_GetRegisteredID
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN void *
NSSGeneralName_GetSpecifiedChoice
(
  NSSGeneralName *generalName,
  NSSGeneralNameChoice choice,
  NSSArena *arenaOpt
);





















NSS_EXTERN PRStatus
NSSGeneralName_Compare
(
  NSSGeneralName *generalName1,
  NSSGeneralName *generalName2,
  PRBool *equalp
);



















NSS_EXTERN NSSGeneralName *
NSSGeneralName_Duplicate
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);



























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetUID
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);


























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetEmail
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetCommonName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetOrganization
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 ** 
NSSGeneralName_GetOrganizationalUnits
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetStateOrProvince
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetLocality
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetCountry
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
NSSGeneralName_GetAttribute
(
  NSSGeneralName *generalName,
  NSSOID *attribute,
  NSSArena *arenaOpt
);





































NSS_EXTERN NSSGeneralNameSeq *
NSSGeneralNameSeq_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berGeneralNameSeq
);




















NSS_EXTERN NSSGeneralNameSeq *
NSSGeneralNameSeq_Create
(
  NSSArena *arenaOpt,
  NSSGeneralName *generalName1,
  ...
);



















NSS_EXTERN PRStatus
NSSGeneralNameSeq_Destroy
(
  NSSGeneralNameSeq *generalNameSeq
);



















NSS_EXTERN NSSDER *
NSSGeneralNameSeq_GetDEREncoding
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSArena *arenaOpt
);























NSS_EXTERN PRStatus
NSSGeneralNameSeq_AppendGeneralName
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSGeneralName *generalName
);
















NSS_EXTERN PRUint32
NSSGeneralNameSeq_GetGeneralNameCount
(
  NSSGeneralNameSeq *generalNameSeq
);
























NSS_EXTERN NSSGeneralName *
NSSGeneralNameSeq_GetGeneralName
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSArena *arenaOpt,
  PRUint32 i
);






















NSS_EXTERN PRStatus
NSSGeneralNameSeq_Compare
(
  NSSGeneralNameSeq *generalNameSeq1,
  NSSGeneralNameSeq *generalNameSeq2,
  PRBool *equalp
);



















NSS_EXTERN NSSGeneralNameSeq *
NSSGeneralNameSeq_Duplicate
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSArena *arenaOpt
);

PR_END_EXTERN_C

#endif 
