



































#ifndef PKI1_H
#define PKI1_H

#ifdef DEBUG
static const char PKI1_CVS_ID[] = "@(#) $RCSfile: pki1.h,v $ $Revision: 1.5 $ $Date: 2005/03/14 18:02:00 $";
#endif 








#ifndef PKI1T_H
#include "pki1t.h"
#endif 

#ifndef NSSPKI1_H
#include "nsspki1.h"
#endif 

PR_BEGIN_EXTERN_C

extern const NSSOID nss_builtin_oids[];
extern const PRUint32 nss_builtin_oid_count;

extern const nssAttributeTypeAliasTable nss_attribute_type_aliases[];
extern const PRUint32 nss_attribute_type_alias_count;




































NSS_EXTERN NSSOID *
nssOID_CreateFromBER
(
  NSSBER *berOid
);

extern const NSSError NSS_ERROR_INVALID_BER;
extern const NSSError NSS_ERROR_NO_MEMORY;



















NSS_EXTERN NSSOID *
nssOID_CreateFromUTF8
(
  NSSUTF8 *stringOid
);

extern const NSSError NSS_ERROR_INVALID_UTF8;
extern const NSSError NSS_ERROR_NO_MEMORY;



















NSS_EXTERN NSSDER *
nssOID_GetDEREncoding
(
  const NSSOID *oid,
  NSSDER *rvOpt,
  NSSArena *arenaOpt
);





















NSS_EXTERN NSSUTF8 *
nssOID_GetUTF8Encoding
(
  const NSSOID *oid,
  NSSArena *arenaOpt
);


















#ifdef DEBUG
NSS_EXTERN PRStatus
nssOID_verifyPointer
(
  const NSSOID *oid
);

extern const NSSError NSS_ERROR_INVALID_NSSOID;
#endif 

























#ifdef DEBUG
NSS_EXTERN const NSSUTF8 *
nssOID_getExplanation
(
  NSSOID *oid
);

extern const NSSError NSS_ERROR_INVALID_NSSOID;
#endif 



























#ifdef DEBUG
NSS_EXTERN NSSUTF8 *
nssOID_getTaggedUTF8
(
  NSSOID *oid,
  NSSArena *arenaOpt
);

extern const NSSError NSS_ERROR_INVALID_NSSOID;
extern const NSSError NSS_ERROR_NO_MEMORY;
#endif 









































NSS_EXTERN NSSATAV *
nssATAV_CreateFromBER
(
  NSSArena *arenaOpt,
  const NSSBER *berATAV
);





















NSS_EXTERN NSSATAV *
nssATAV_CreateFromUTF8
(
  NSSArena *arenaOpt,
  const NSSUTF8 *stringATAV
);
























NSS_EXTERN NSSATAV *
nssATAV_Create
(
  NSSArena *arenaOpt,
  const NSSOID *oid,
  const void *data,
  PRUint32 length
);



















NSS_EXTERN PRStatus
nssATAV_Destroy
(
  NSSATAV *atav
);



















NSS_EXTERN NSSDER *
nssATAV_GetDEREncoding
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);





















NSS_EXTERN NSSUTF8 *
nssATAV_GetUTF8Encoding
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);

















NSS_EXTERN const NSSOID *
nssATAV_GetType
(
  NSSATAV *atav
);




















NSS_EXTERN NSSUTF8 *
nssATAV_GetValue
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);





















NSS_EXTERN PRStatus
nssATAV_Compare
(
  NSSATAV *atav1,
  NSSATAV *atav2,
  PRBool *equalp
);



















NSS_EXTERN NSSATAV *
nssATAV_Duplicate
(
  NSSATAV *atav,
  NSSArena *arenaOpt
);



















#ifdef DEBUG
NSS_EXTERN PRStatus
nssATAV_verifyPointer
(
  NSSATAV *atav
);
#endif 








































NSS_EXTERN NSSRDN *
nssRDN_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berRDN
);























NSS_EXTERN NSSRDN *
nssRDN_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringRDN
);




















NSS_EXTERN NSSRDN *
nssRDN_Create
(
  NSSArena *arenaOpt,
  NSSATAV *atav1,
  ...
);



















NSS_EXTERN NSSRDN *
nssRDN_CreateSimple
(
  NSSArena *arenaOpt,
  NSSATAV *atav
);



















NSS_EXTERN PRStatus
nssRDN_Destroy
(
  NSSRDN *rdn
);



















NSS_EXTERN NSSDER *
nssRDN_GetDEREncoding
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);






















NSS_EXTERN NSSUTF8 *
nssRDN_GetUTF8Encoding
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);























NSS_EXTERN PRStatus
nssRDN_AddATAV
(
  NSSRDN *rdn,
  NSSATAV *atav
);
















NSS_EXTERN PRUint32
nssRDN_GetATAVCount
(
  NSSRDN *rdn
);



























NSS_EXTERN NSSATAV *
nssRDN_GetATAV
(
  NSSRDN *rdn,
  NSSArena *arenaOpt,
  PRUint32 i
);























NSS_EXTERN NSSATAV *
nssRDN_GetSimpleATAV
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);






















NSS_EXTERN PRStatus
nssRDN_Compare
(
  NSSRDN *rdn1,
  NSSRDN *rdn2,
  PRBool *equalp
);



















NSS_EXTERN NSSRDN *
nssRDN_Duplicate
(
  NSSRDN *rdn,
  NSSArena *arenaOpt
);









































NSS_EXTERN NSSRDNSeq *
nssRDNSeq_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berRDNSeq
);






















NSS_EXTERN NSSRDNSeq *
nssRDNSeq_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringRDNSeq
);




















NSS_EXTERN NSSRDNSeq *
nssRDNSeq_Create
(
  NSSArena *arenaOpt,
  NSSRDN *rdn1,
  ...
);



















NSS_EXTERN PRStatus
nssRDNSeq_Destroy
(
  NSSRDNSeq *rdnseq
);



















NSS_EXTERN NSSDER *
nssRDNSeq_GetDEREncoding
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt
);




















NSS_EXTERN NSSUTF8 *
nssRDNSeq_GetUTF8Encoding
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt
);























NSS_EXTERN PRStatus
nssRDNSeq_AppendRDN
(
  NSSRDNSeq *rdnseq,
  NSSRDN *rdn
);
















NSS_EXTERN PRUint32
nssRDNSeq_GetRDNCount
(
  NSSRDNSeq *rdnseq
);


























NSS_EXTERN NSSRDN *
nssRDNSeq_GetRDN
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt,
  PRUint32 i
);





















NSS_EXTERN PRStatus
nssRDNSeq_Compare
(
  NSSRDNSeq *rdnseq1,
  NSSRDNSeq *rdnseq2,
  PRBool *equalp
);



















NSS_EXTERN NSSRDNSeq *
nssRDNSeq_Duplicate
(
  NSSRDNSeq *rdnseq,
  NSSArena *arenaOpt
);


















NSS_EXTERN PRBool
nssRDNSeq_EvaluateUTF8
(
  NSSUTF8 *str
);



















































NSS_EXTERN NSSName *
nssName_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berName
);





















NSS_EXTERN NSSName *
nssName_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringName
);




























NSS_EXTERN NSSName *
nssName_Create
(
  NSSArena *arenaOpt,
  NSSNameChoice choice,
  void *arg
);



















NSS_EXTERN PRStatus
nssName_Destroy
(
  NSSName *name
);



















NSS_EXTERN NSSDER *
nssName_GetDEREncoding
(
  NSSName *name,
  NSSArena *arenaOpt
);





















NSS_EXTERN NSSUTF8 *
nssName_GetUTF8Encoding
(
  NSSName *name,
  NSSArena *arenaOpt
);

















NSS_EXTERN NSSNameChoice
nssName_GetChoice
(
  NSSName *name
);
























NSS_EXTERN NSSRDNSeq *
nssName_GetRDNSequence
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN void *
nssName_GetSpecifiedChoice
(
  NSSName *name,
  NSSNameChoice choice,
  NSSArena *arenaOpt
);





















NSS_EXTERN PRStatus
nssName_Compare
(
  NSSName *name1,
  NSSName *name2,
  PRBool *equalp
);



















NSS_EXTERN NSSName *
nssName_Duplicate
(
  NSSName *name,
  NSSArena *arenaOpt
);


























NSS_EXTERN NSSUTF8 * 
nssName_GetUID
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssName_GetEmail
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
nssName_GetCommonName
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
nssName_GetOrganization
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 ** 
nssName_GetOrganizationalUnits
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
nssName_GetStateOrProvince
(
  NSSName *name,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSUTF8 * 
nssName_GetLocality
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssName_GetCountry
(
  NSSName *name,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssName_GetAttribute
(
  NSSName *name,
  NSSOID *attribute,
  NSSArena *arenaOpt
);


















NSS_EXTERN PRBool
nssName_EvaluateUTF8
(
  NSSUTF8 *str
);

























































NSS_EXTERN NSSGeneralName *
nssGeneralName_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berGeneralName
);




















NSS_EXTERN NSSGeneralName *
nssGeneralName_CreateFromUTF8
(
  NSSArena *arenaOpt,
  NSSUTF8 *stringGeneralName
);




































NSS_EXTERN NSSGeneralName *
nssGeneralName_Create
(
  NSSGeneralNameChoice choice,
  void *arg
);




















NSS_EXTERN PRStatus
nssGeneralName_Destroy
(
  NSSGeneralName *generalName
);



















NSS_EXTERN NSSDER *
nssGeneralName_GetDEREncoding
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);





















NSS_EXTERN NSSUTF8 *
nssGeneralName_GetUTF8Encoding
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);


















NSS_EXTERN NSSGeneralNameChoice
nssGeneralName_GetChoice
(
  NSSGeneralName *generalName
);























NSS_EXTERN NSSOtherName *
nssGeneralName_GetOtherName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSRFC822Name *
nssGeneralName_GetRfc822Name
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSDNSName *
nssGeneralName_GetDNSName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSX400Address *
nssGeneralName_GetX400Address
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSName *
nssGeneralName_GetName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSEdiPartyName *
nssGeneralName_GetEdiPartyName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSURI *
nssGeneralName_GetUniformResourceIdentifier
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSIPAddress *
nssGeneralName_GetIPAddress
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN NSSRegisteredID *
nssGeneralName_GetRegisteredID
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);























NSS_EXTERN void *
nssGeneralName_GetSpecifiedChoice
(
  NSSGeneralName *generalName,
  NSSGeneralNameChoice choice,
  NSSArena *arenaOpt
);





















NSS_EXTERN PRStatus
nssGeneralName_Compare
(
  NSSGeneralName *generalName1,
  NSSGeneralName *generalName2,
  PRBool *equalp
);



















NSS_EXTERN NSSGeneralName *
nssGeneralName_Duplicate
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);



























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetUID
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);


























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetEmail
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetCommonName
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetOrganization
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);

























NSS_EXTERN NSSUTF8 ** 
nssGeneralName_GetOrganizationalUnits
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetStateOrProvince
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetLocality
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetCountry
(
  NSSGeneralName *generalName,
  NSSArena *arenaOpt
);
























NSS_EXTERN NSSUTF8 * 
nssGeneralName_GetAttribute
(
  NSSGeneralName *generalName,
  NSSOID *attribute,
  NSSArena *arenaOpt
);





































NSS_EXTERN NSSGeneralNameSeq *
nssGeneralNameSeq_CreateFromBER
(
  NSSArena *arenaOpt,
  NSSBER *berGeneralNameSeq
);




















NSS_EXTERN NSSGeneralNameSeq *
nssGeneralNameSeq_Create
(
  NSSArena *arenaOpt,
  NSSGeneralName *generalName1,
  ...
);




















NSS_EXTERN PRStatus
nssGeneralNameSeq_Destroy
(
  NSSGeneralNameSeq *generalNameSeq
);



















NSS_EXTERN NSSDER *
nssGeneralNameSeq_GetDEREncoding
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSArena *arenaOpt
);























NSS_EXTERN PRStatus
nssGeneralNameSeq_AppendGeneralName
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSGeneralName *generalName
);
















NSS_EXTERN PRUint32
nssGeneralNameSeq_GetGeneralNameCount
(
  NSSGeneralNameSeq *generalNameSeq
);
























NSS_EXTERN NSSGeneralName *
nssGeneralNameSeq_GetGeneralName
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSArena *arenaOpt,
  PRUint32 i
);






















NSS_EXTERN PRStatus
nssGeneralNameSeq_Compare
(
  NSSGeneralNameSeq *generalNameSeq1,
  NSSGeneralNameSeq *generalNameSeq2,
  PRBool *equalp
);



















NSS_EXTERN NSSGeneralNameSeq *
nssGeneralNameSeq_Duplicate
(
  NSSGeneralNameSeq *generalNameSeq,
  NSSArena *arenaOpt
);

PR_END_EXTERN_C

#endif 
