



#include "secoid.h"
#include "secmodt.h" 

#define OI(x) { siDEROID, (unsigned char *)x, sizeof x }
#define OD(oid,tag,desc,mech,ext) { OI(oid), tag, desc, mech, ext }
#define ODN(oid,desc) \
  { OI(oid), 0, desc, CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION }

#define OIDT static const unsigned char


#define OIWSSIG   0x2B, 13, 3, 2

OIDT  oiwMD5RSA[] 	= { OIWSSIG,  3 };
OIDT  oiwDESCBC[] 	= { OIWSSIG,  7 };
OIDT  oiwRSAsig[] 	= { OIWSSIG, 11 };
OIDT  oiwDSA   [] 	= { OIWSSIG, 12 };
OIDT  oiwMD5RSAsig[] 	= { OIWSSIG, 25 };
OIDT  oiwSHA1  [] 	= { OIWSSIG, 26 };
OIDT  oiwDSASHA1[] 	= { OIWSSIG, 27 };
OIDT  oiwDSASHA1param[] = { OIWSSIG, 28 };
OIDT  oiwSHA1RSA[] 	= { OIWSSIG, 29 };



#define MICROSOFT 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37

OIDT  mCTL[] 	= { MICROSOFT, 10, 3, 1 }; 
OIDT  mTSS[] 	= { MICROSOFT, 10, 3, 2 }; 
OIDT  mSGC[] 	= { MICROSOFT, 10, 3, 3 }; 
OIDT  mEFS[]	= { MICROSOFT, 10, 3, 4 }; 
OIDT  mSMIME[]	= { MICROSOFT, 16, 4    }; 

OIDT  mECRTT[]	= { MICROSOFT, 20, 2    }; 
OIDT  mEAGNT[]	= { MICROSOFT, 20, 2, 1 }; 
OIDT  mKPSCL[]	= { MICROSOFT, 20, 2, 2 }; 
OIDT  mNTPN []	= { MICROSOFT, 20, 2, 3 }; 
OIDT  mCASRV[]	= { MICROSOFT, 21, 1    }; 


#define AOL 0x2B, 0x06, 0x01, 0x04, 0x01, 0x88, 0x2A


#define ID_PKIX 0x2B, 6, 1, 5, 5, 7

#define ID_AD   ID_PKIX, 48

OIDT  padOCSP[]      = { ID_AD, 1 };  
OIDT  padCAissuer[]  = { ID_AD, 2 };  
OIDT  padTimeStamp[] = { ID_AD, 3 };  


#define X500                    0x55
#define X520_ATTRIBUTE_TYPE     X500, 0x04
#define X500_ALG                X500, 0x08
#define X500_ALG_ENCRYPTION     X500_ALG, 0x01
#define ID_CE			X500, 29

OIDT cePlcyObs[] = { ID_CE,  3 };  
OIDT cePlcyCns[] = { ID_CE, 36 };  


#define USCOM        0x60, 0x86, 0x48, 0x01
#define USGOV        USCOM, 0x65
#define USDOD        USGOV, 2
#define ID_INFOSEC   USDOD, 1


#define VERISIGN_PKI USCOM, 0x86, 0xf8, 0x45, 1
#define VERISIGN_XTN VERISIGN_PKI, 6
#define VERISIGN_POL VERISIGN_PKI, 7	/* Cert policies */
#define VERISIGN_TNET VERISIGN_POL, 23	/* Verisign Trust Network */

OIDT  vcx7[]	= { VERISIGN_XTN, 7 };	
OIDT  vcp1[]	= { VERISIGN_TNET, 1 };	
OIDT  vcp2[]	= { VERISIGN_TNET, 2 };	
OIDT  vcp3[]	= { VERISIGN_TNET, 3 };	
OIDT  vcp4[]	= { VERISIGN_TNET, 4 };	



static const SECOidData oids[] = {

    ODN( oiwMD5RSA,	  "OIWSecSIG MD5 with RSA"),
    ODN( oiwDESCBC,	  "OIWSecSIG DES CBC"),
    ODN( oiwRSAsig,	  "OIWSecSIG RSA signature"),
    ODN( oiwDSA   ,	  "OIWSecSIG DSA"),
    ODN( oiwMD5RSAsig,	  "OIWSecSIG MD5 with RSA signature"),
    ODN( oiwSHA1  ,	  "OIWSecSIG SHA1"),
    ODN( oiwDSASHA1,	  "OIWSecSIG DSA with SHA1"),
    ODN( oiwDSASHA1param, "OIWSecSIG DSA with SHA1 with params"),
    ODN( oiwSHA1RSA,	  "OIWSecSIG MD5 with RSA"),


    ODN( mCTL,   "Microsoft Cert Trust List signing"), 
    ODN( mTSS,   "Microsoft Time Stamp signing"),
    ODN( mSGC,   "Microsoft SGC SSL server"),
    ODN( mEFS,   "Microsoft Encrypted File System"),
    ODN( mSMIME, "Microsoft SMIME preferences"),
    ODN( mECRTT, "Microsoft Enrollment Cert Type Extension"),
    ODN( mEAGNT, "Microsoft Enrollment Agent"),
    ODN( mKPSCL, "Microsoft KP SmartCard Logon"),
    ODN( mNTPN,  "Microsoft NT Principal Name"),
    ODN( mCASRV, "Microsoft CertServ CA version"),


    ODN( padOCSP,	"PKIX OCSP method"),
    ODN( padCAissuer,	"PKIX CA Issuer method"),
    ODN( padTimeStamp,	"PKIX Time Stamping method"),


    ODN( cePlcyObs,	"Certificate Policies (Obsolete)"),
    ODN( cePlcyCns,	"Certificate Policy Constraints"),


    ODN( vcx7,		"Verisign Cert Extension 7 (?)"),
    ODN( vcp1,		"Verisign Class 1 Certificate Policy"),
    ODN( vcp2,		"Verisign Class 2 Certificate Policy"),
    ODN( vcp3,		"Verisign Class 3 Certificate Policy"),
    ODN( vcp4,		"Verisign Class 4 Certificate Policy"),

};

static const unsigned int numOids = (sizeof oids) / (sizeof oids[0]);

SECStatus
SECU_RegisterDynamicOids(void)
{
    unsigned int i;
    SECStatus rv = SECSuccess;

    for (i = 0; i < numOids; ++i) {
	SECOidTag tag = SECOID_AddEntry(&oids[i]);
	if (tag == SEC_OID_UNKNOWN) {
	    rv = SECFailure;
#ifdef DEBUG_DYN_OIDS
	    fprintf(stderr, "Add OID[%d] failed\n", i);
	} else {
	    fprintf(stderr, "Add OID[%d] returned tag %d\n", i, tag);
#endif
	}
    }
    return rv;
}
