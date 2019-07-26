















#ifndef __USPREP_H__
#define __USPREP_H__






#include "unicode/utypes.h"
#include "unicode/localpointer.h"

































#if !UCONFIG_NO_IDNA

#include "unicode/parseerr.h"





typedef struct UStringPrepProfile UStringPrepProfile;








#define USPREP_DEFAULT 0x0000







#define USPREP_ALLOW_UNASSIGNED 0x0001







typedef enum UStringPrepProfileType {
    



    USPREP_RFC3491_NAMEPREP,
    



	USPREP_RFC3530_NFS4_CS_PREP,
    



	USPREP_RFC3530_NFS4_CS_PREP_CI,
    



	USPREP_RFC3530_NFS4_CIS_PREP,
    



	USPREP_RFC3530_NFS4_MIXED_PREP_PREFIX,
    



	USPREP_RFC3530_NFS4_MIXED_PREP_SUFFIX,
    



	USPREP_RFC3722_ISCSI,
    



	USPREP_RFC3920_NODEPREP,
    



	USPREP_RFC3920_RESOURCEPREP,
    



	USPREP_RFC4011_MIB,
    



    USPREP_RFC4013_SASLPREP,
    



	USPREP_RFC4505_TRACE,
    



	USPREP_RFC4518_LDAP,
    




	USPREP_RFC4518_LDAP_CI
} UStringPrepProfileType;
















U_STABLE UStringPrepProfile* U_EXPORT2
usprep_open(const char* path, 
            const char* fileName,
            UErrorCode* status);












U_STABLE UStringPrepProfile* U_EXPORT2
usprep_openByType(UStringPrepProfileType type,
				  UErrorCode* status);






U_STABLE void U_EXPORT2
usprep_close(UStringPrepProfile* profile);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUStringPrepProfilePointer, UStringPrepProfile, usprep_close);

U_NAMESPACE_END

#endif






























U_STABLE int32_t U_EXPORT2
usprep_prepare(   const UStringPrepProfile* prep,
                  const UChar* src, int32_t srcLength, 
                  UChar* dest, int32_t destCapacity,
                  int32_t options,
                  UParseError* parseError,
                  UErrorCode* status );


#endif 

#endif
