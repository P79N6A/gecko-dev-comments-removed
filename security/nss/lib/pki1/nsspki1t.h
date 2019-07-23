



































#ifndef NSSPKI1T_H
#define NSSPKI1T_H

#ifdef DEBUG
static const char NSSPKI1T_CVS_ID[] = "@(#) $RCSfile: nsspki1t.h,v $ $Revision: 1.3 $ $Date: 2005/01/20 02:25:49 $";
#endif 








#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

PR_BEGIN_EXTERN_C







struct NSSOIDStr;
typedef struct NSSOIDStr NSSOID;












struct NSSATAVStr;
typedef struct NSSATAVStr NSSATAV;


















struct NSSRDNStr;
typedef struct NSSRDNStr NSSRDN;








struct NSSRDNSeqStr;
typedef struct NSSRDNSeqStr NSSRDNSeq;








struct NSSNameStr;
typedef struct NSSNameStr NSSName;







enum NSSNameChoiceEnum {
  NSSNameChoiceInvalid = -1,
  NSSNameChoiceRdnSequence
};
typedef enum NSSNameChoiceEnum NSSNameChoice;








struct NSSGeneralNameStr;
typedef struct NSSGeneralNameStr NSSGeneralName;







enum NSSGeneralNameChoiceEnum {
  NSSGeneralNameChoiceInvalid = -1,
  NSSGeneralNameChoiceOtherName = 0,
  NSSGeneralNameChoiceRfc822Name = 1,
  NSSGeneralNameChoiceDNSName = 2,
  NSSGeneralNameChoiceX400Address = 3,
  NSSGeneralNameChoiceDirectoryName = 4,
  NSSGeneralNameChoiceEdiPartyName = 5,
  NSSGeneralNameChoiceUniformResourceIdentifier = 6,
  NSSGeneralNameChoiceIPAddress = 7,
  NSSGeneralNameChoiceRegisteredID = 8
};
typedef enum NSSGeneralNameChoiceEnum NSSGeneralNameChoice;





struct NSSOtherNameStr;
typedef struct NSSOtherNameStr NSSOtherName;

struct NSSRFC822NameStr;
typedef struct NSSRFC822NameStr NSSRFC822Name;

struct NSSDNSNameStr;
typedef struct NSSDNSNameStr NSSDNSName;

struct NSSX400AddressStr;
typedef struct NSSX400AddressStr NSSX400Address;

struct NSSEdiPartyNameStr;
typedef struct NSSEdiPartyNameStr NSSEdiPartyName;

struct NSSURIStr;
typedef struct NSSURIStr NSSURI;

struct NSSIPAddressStr;
typedef struct NSSIPAddressStr NSSIPAddress;

struct NSSRegisteredIDStr;
typedef struct NSSRegisteredIDStr NSSRegisteredID;











struct NSSGeneralNameSeqStr;
typedef struct NSSGeneralNameSeqStr NSSGeneralNameSeq;

PR_END_EXTERN_C

#endif 
