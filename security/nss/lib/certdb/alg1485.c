





































#include "prprf.h"
#include "cert.h"
#include "certi.h"
#include "xconst.h"
#include "genname.h"
#include "secitem.h"
#include "secerr.h"

typedef struct NameToKindStr {
    const char * name;
    unsigned int maxLen; 
    SECOidTag    kind;
    int		 valueType;
} NameToKind;


#define SEC_ASN1_DS SEC_ASN1_HIGH_TAG_NUMBER


static const NameToKind name2kinds[] = {




    { "CN",             64, SEC_OID_AVA_COMMON_NAME,    SEC_ASN1_DS},
    { "ST",            128, SEC_OID_AVA_STATE_OR_PROVINCE,
							SEC_ASN1_DS},
    { "O",              64, SEC_OID_AVA_ORGANIZATION_NAME,
							SEC_ASN1_DS},
    { "OU",             64, SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME,
                                                        SEC_ASN1_DS},
    { "dnQualifier", 32767, SEC_OID_AVA_DN_QUALIFIER, SEC_ASN1_PRINTABLE_STRING},
    { "C",               2, SEC_OID_AVA_COUNTRY_NAME, SEC_ASN1_PRINTABLE_STRING},
    { "serialNumber",   64, SEC_OID_AVA_SERIAL_NUMBER,SEC_ASN1_PRINTABLE_STRING},


    { "L",             128, SEC_OID_AVA_LOCALITY,       SEC_ASN1_DS},
    { "title",          64, SEC_OID_AVA_TITLE,          SEC_ASN1_DS},
    { "SN",             64, SEC_OID_AVA_SURNAME,        SEC_ASN1_DS},
    { "givenName",      64, SEC_OID_AVA_GIVEN_NAME,     SEC_ASN1_DS},
    { "initials",       64, SEC_OID_AVA_INITIALS,       SEC_ASN1_DS},
    { "generationQualifier",
                        64, SEC_OID_AVA_GENERATION_QUALIFIER,
                                                        SEC_ASN1_DS},

    { "DC",            128, SEC_OID_AVA_DC,             SEC_ASN1_IA5_STRING},
    { "MAIL",          256, SEC_OID_RFC1274_MAIL,       SEC_ASN1_IA5_STRING},
    { "UID",           256, SEC_OID_RFC1274_UID,        SEC_ASN1_DS},











    { "postalAddress", 128, SEC_OID_AVA_POSTAL_ADDRESS, SEC_ASN1_DS},
    { "postalCode",     40, SEC_OID_AVA_POSTAL_CODE,    SEC_ASN1_DS},
    { "postOfficeBox",  40, SEC_OID_AVA_POST_OFFICE_BOX,SEC_ASN1_DS},
    { "houseIdentifier",64, SEC_OID_AVA_HOUSE_IDENTIFIER,SEC_ASN1_DS},



    { "E",             128, SEC_OID_PKCS9_EMAIL_ADDRESS,SEC_ASN1_DS},

#if 0 
    { "pseudonym",      64, SEC_OID_AVA_PSEUDONYM,      SEC_ASN1_DS},
#endif

    { 0,           256, SEC_OID_UNKNOWN                      , 0},
};

#define C_DOUBLE_QUOTE '\042'

#define C_BACKSLASH '\134'

#define C_EQUAL '='

#define OPTIONAL_SPACE(c) \
    (((c) == ' ') || ((c) == '\r') || ((c) == '\n'))

#define SPECIAL_CHAR(c)						\
    (((c) == ',') || ((c) == '=') || ((c) == C_DOUBLE_QUOTE) ||	\
     ((c) == '\r') || ((c) == '\n') || ((c) == '+') ||		\
     ((c) == '<') || ((c) == '>') || ((c) == '#') ||		\
     ((c) == ';') || ((c) == C_BACKSLASH))


#define IS_PRINTABLE(c)						\
    ((((c) >= 'a') && ((c) <= 'z')) ||				\
     (((c) >= 'A') && ((c) <= 'Z')) ||				\
     (((c) >= '0') && ((c) <= '9')) ||				\
     ((c) == ' ') ||						\
     ((c) == '\'') ||						\
     ((c) == '\050') ||				/* ( */		\
     ((c) == '\051') ||				/* ) */		\
     (((c) >= '+') && ((c) <= '/')) ||		/* + , - . / */	\
     ((c) == ':') ||						\
     ((c) == '=') ||						\
     ((c) == '?'))

int
cert_AVAOidTagToMaxLen(SECOidTag tag)
{
    const NameToKind *n2k = name2kinds;

    while (n2k->kind != tag && n2k->kind != SEC_OID_UNKNOWN) {
	++n2k;
    }
    return (n2k->kind != SEC_OID_UNKNOWN) ? n2k->maxLen : -1;
}

static PRBool
IsPrintable(unsigned char *data, unsigned len)
{
    unsigned char ch, *end;

    end = data + len;
    while (data < end) {
	ch = *data++;
	if (!IS_PRINTABLE(ch)) {
	    return PR_FALSE;
	}
    }
    return PR_TRUE;
}

static void
skipSpace(char **pbp, char *endptr)
{
    char *bp = *pbp;
    while (bp < endptr && OPTIONAL_SPACE(*bp)) {
	bp++;
    }
    *pbp = bp;
}

static SECStatus
scanTag(char **pbp, char *endptr, char *tagBuf, int tagBufSize)
{
    char *bp, *tagBufp;
    int taglen;

    PORT_Assert(tagBufSize > 0);
    
    
    skipSpace(pbp, endptr);
    if (*pbp == endptr) {
	
	return SECFailure;
    }
    
    
    taglen = 0;
    bp = *pbp;
    tagBufp = tagBuf;
    while (bp < endptr && !OPTIONAL_SPACE(*bp) && (*bp != C_EQUAL)) {
	if (++taglen >= tagBufSize) {
	    *pbp = bp;
	    return SECFailure;
	}
	*tagBufp++ = *bp++;
    }
    
    *tagBufp++ = 0;
    *pbp = bp;
    
    
    skipSpace(pbp, endptr);
    if (*pbp == endptr) {
	
	return SECFailure;
    }
    if (**pbp != C_EQUAL) {
	
	return SECFailure;
    }
    
    (*pbp)++;
    
    return SECSuccess;
}

static SECStatus
scanVal(char **pbp, char *endptr, char *valBuf, int valBufSize)  
{
    char *bp, *valBufp;
    int vallen;
    PRBool isQuoted;
    
    PORT_Assert(valBufSize > 0);
    
    
    skipSpace(pbp, endptr);
    if(*pbp == endptr) {
	
	return SECFailure;
    }
    
    bp = *pbp;
    
    
    if (*bp == C_DOUBLE_QUOTE) {
	isQuoted = PR_TRUE;
	
	bp++;
    } else {
	isQuoted = PR_FALSE;
    }
    
    valBufp = valBuf;
    vallen = 0;
    while (bp < endptr) {
	char c = *bp;
	if (c == C_BACKSLASH) {
	    
	    bp++;
	    if (bp >= endptr) {
		
		*pbp = bp;
		return SECFailure;
	    }
	} else if (c == '#' && bp == *pbp) {
	    
	} else if (!isQuoted && SPECIAL_CHAR(c)) {
	    
	    break;
	} else if (c == C_DOUBLE_QUOTE) {
	    
	    break;
	}
	
        vallen++;
	if (vallen >= valBufSize) {
	    *pbp = bp;
	    return SECFailure;
	}
	*valBufp++ = *bp++;
    }
    
    
    if (!isQuoted) {
	if (valBufp > valBuf) {
	    valBufp--;
	    while ((valBufp > valBuf) && OPTIONAL_SPACE(*valBufp)) {
		valBufp--;
	    }
	    valBufp++;
	}
    }
    
    if (isQuoted) {
	
	if (*bp != C_DOUBLE_QUOTE) {
	    *pbp = bp;
	    return SECFailure;
	}
	
	bp++;
	skipSpace(&bp, endptr);
    }
    
    *pbp = bp;
    
    if (valBufp == valBuf) {
	
	return SECFailure;
    }
    
    
    *valBufp++ = 0;
    
    return SECSuccess;
}

static const PRInt16 x2b[256] = 
{
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, 
 -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


static SECStatus
hexToBin(PLArenaPool *pool, SECItem * destItem, const char * src, int len)
{
    PRUint8 * dest;

    destItem->data = NULL; 
    if (len <= 0 || (len & 1)) {
	goto loser;
    }
    len >>= 1;
    if (!SECITEM_AllocItem(pool, destItem, len))
	goto loser;
    dest = destItem->data;
    for (; len > 0; len--, src += 2) {
	PRInt16 bin = (x2b[(PRUint8)src[0]] << 4) | x2b[(PRUint8)src[1]]; 
	if (bin < 0)
	    goto loser;
	*dest++ = (PRUint8)bin;
    }
    return SECSuccess;
loser:
    if (!pool)
    	SECITEM_FreeItem(destItem, PR_FALSE);
    return SECFailure;
}








static CERTAVA *
ParseRFC1485AVA(PRArenaPool *arena, char **pbp, char *endptr)
{
    CERTAVA *a;
    const NameToKind *n2k;
    char *bp;
    int       vt = -1;
    int       valLen;
    SECOidTag kind  = SEC_OID_UNKNOWN;
    SECStatus rv    = SECFailure;
    SECItem   derOid = { 0, NULL, 0 };
    char      sep   = 0;

    char tagBuf[32];
    char valBuf[384];

    PORT_Assert(arena);
    if (scanTag(pbp, endptr, tagBuf, sizeof(tagBuf)) == SECFailure ||
	scanVal(pbp, endptr, valBuf, sizeof(valBuf)) == SECFailure) {
	goto loser;
    }

    bp = *pbp;
    if (bp < endptr) {
	sep = *bp++; 
    }
    *pbp = bp;
    
    if (sep && sep != ',' && sep != ';' && sep != '+') {
	goto loser;
    }

    
    if (!PL_strncasecmp("oid.", tagBuf, 4)) {
        rv = SEC_StringToOID(arena, &derOid, tagBuf, strlen(tagBuf));
    } else {
	for (n2k = name2kinds; n2k->name; n2k++) {
	    SECOidData *oidrec;
	    if (PORT_Strcasecmp(n2k->name, tagBuf) == 0) {
		kind = n2k->kind;
		vt   = n2k->valueType;
		oidrec = SECOID_FindOIDByTag(kind);
		if (oidrec == NULL)
		    goto loser;
		derOid = oidrec->oid;
		break;
	    }
	}
    }
    if (kind == SEC_OID_UNKNOWN && rv != SECSuccess) 
	goto loser;

    
    if ('#' == valBuf[0]) {
    	
	SECItem  derVal = { 0, NULL, 0};
	valLen = PORT_Strlen(valBuf+1);
	rv = hexToBin(arena, &derVal, valBuf + 1, valLen);
	if (rv)
	    goto loser;
	a = CERT_CreateAVAFromRaw(arena, &derOid, &derVal);
    } else {
	if (kind == SEC_OID_UNKNOWN)
	    goto loser;
	valLen = PORT_Strlen(valBuf);
	if (kind == SEC_OID_AVA_COUNTRY_NAME && valLen != 2)
	    goto loser;
	if (vt == SEC_ASN1_PRINTABLE_STRING &&
	    !IsPrintable((unsigned char*) valBuf, valLen)) 
	    goto loser;
	if (vt == SEC_ASN1_DS) {
	    
	    if (IsPrintable((unsigned char*) valBuf, valLen))
		vt = SEC_ASN1_PRINTABLE_STRING;
	    else 
		vt = SEC_ASN1_UTF8_STRING;
	}

	a = CERT_CreateAVA(arena, kind, vt, (char *)valBuf);
    }
    return a;

loser:
    
    PORT_SetError(SEC_ERROR_INVALID_AVA);
    return 0;
}

static CERTName *
ParseRFC1485Name(char *buf, int len)
{
    SECStatus rv;
    CERTName *name;
    char *bp, *e;
    CERTAVA *ava;
    CERTRDN *rdn = NULL;

    name = CERT_CreateName(NULL);
    if (name == NULL) {
	return NULL;
    }
    
    e = buf + len;
    bp = buf;
    while (bp < e) {
	ava = ParseRFC1485AVA(name->arena, &bp, e);
	if (ava == 0) 
	    goto loser;
	if (!rdn) {
	    rdn = CERT_CreateRDN(name->arena, ava, (CERTAVA *)0);
	    if (rdn == 0) 
		goto loser;
	    rv = CERT_AddRDN(name, rdn);
	} else {
	    rv = CERT_AddAVA(name->arena, rdn, ava);
	}
	if (rv) 
	    goto loser;
	if (bp[-1] != '+')
	    rdn = NULL; 
	skipSpace(&bp, e);
    }

    if (name->rdns[0] == 0) {
	
	goto loser;
    }

    
    {
	CERTRDN **firstRdn;
	CERTRDN **lastRdn;
	CERTRDN *tmp;
	
	
	firstRdn = name->rdns;
	
	
	lastRdn = name->rdns;
	while (*lastRdn) lastRdn++;
	lastRdn--;
	
	
	for ( ; firstRdn < lastRdn; firstRdn++, lastRdn--) {
	    tmp = *firstRdn;
	    *firstRdn = *lastRdn;
	    *lastRdn = tmp;
	}
    }
    
    
    return name;
    
  loser:
    CERT_DestroyName(name);
    return NULL;
}

CERTName *
CERT_AsciiToName(char *string)
{
    CERTName *name;
    name = ParseRFC1485Name(string, PORT_Strlen(string));
    return name;
}



typedef struct stringBufStr {
    char *buffer;
    unsigned offset;
    unsigned size;
} stringBuf;

#define DEFAULT_BUFFER_SIZE 200

static SECStatus
AppendStr(stringBuf *bufp, char *str)
{
    char *buf;
    unsigned bufLen, bufSize, len;
    int size = 0;

    
    buf = bufp->buffer;
    bufLen = bufp->offset;
    len = PORT_Strlen(str);
    bufSize = bufLen + len;
    if (!buf) {
	bufSize++;
	size = PR_MAX(DEFAULT_BUFFER_SIZE,bufSize*2);
	buf = (char *) PORT_Alloc(size);
	bufp->size = size;
    } else if (bufp->size < bufSize) {
	size = bufSize*2;
	buf =(char *) PORT_Realloc(buf,size);
	bufp->size = size;
    }
    if (!buf) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    bufp->buffer = buf;
    bufp->offset = bufSize;

    
    buf = buf + bufLen;
    if (bufLen) buf--;			
    PORT_Memcpy(buf, str, len+1);		
    return SECSuccess;
}

static int
cert_RFC1485_GetRequiredLen(const char *src, int srclen, PRBool *pNeedsQuoting)
{
    int i, reqLen=0;
    PRBool needsQuoting = PR_FALSE;
    char lastC = 0;

    
    for (i = 0; i < srclen; i++) {
	char c = src[i];
	reqLen++;
	if (!needsQuoting && (SPECIAL_CHAR(c) ||
	    (OPTIONAL_SPACE(c) && OPTIONAL_SPACE(lastC)))) {
	    
	    needsQuoting = PR_TRUE;
	}
	if (c == C_DOUBLE_QUOTE || c == C_BACKSLASH) {
	    
	    reqLen++;
	}
	lastC = c;
    }
    
    if (!needsQuoting && srclen > 0 && 
	(OPTIONAL_SPACE(src[srclen-1]) || OPTIONAL_SPACE(src[0]))) {
	needsQuoting = PR_TRUE;
    }

    if (needsQuoting) 
    	reqLen += 2;
    if (pNeedsQuoting)
    	*pNeedsQuoting = needsQuoting;

    return reqLen;
}

SECStatus
CERT_RFC1485_EscapeAndQuote(char *dst, int dstlen, char *src, int srclen)
{
    int i, reqLen=0;
    char *d = dst;
    PRBool needsQuoting = PR_FALSE;

    
    reqLen = cert_RFC1485_GetRequiredLen(src, srclen, &needsQuoting) + 1;
    if (reqLen > dstlen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }

    d = dst;
    if (needsQuoting) *d++ = C_DOUBLE_QUOTE;
    for (i = 0; i < srclen; i++) {
	char c = src[i];
	if (c == C_DOUBLE_QUOTE || c == C_BACKSLASH) {
	    
	    *d++ = C_BACKSLASH;
	}
	*d++ = c;
    }
    if (needsQuoting) *d++ = C_DOUBLE_QUOTE;
    *d++ = 0;
    return SECSuccess;
}



char *
CERT_GetOidString(const SECItem *oid)
{
    PRUint8 *end;
    PRUint8 *d;
    PRUint8 *e;
    char *a         = NULL;
    char *b;

#define MAX_OID_LEN 1024 /* bytes */

    if (oid->len > MAX_OID_LEN) {
    	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return NULL;
    }

    
    d = (PRUint8 *)oid->data;
    
    end = &d[ oid->len ];

    


    if( (*d == 0x80) && (2 == oid->len) ) {
	
	a = PR_smprintf("%lu", (PRUint32)d[1]);
	if( (char *)NULL == a ) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    return (char *)NULL;
	}
	return a;
    }

    for( ; d < end; d = &e[1] ) {
    
	for( e = d; e < end; e++ ) {
	    if( 0 == (*e & 0x80) ) {
		break;
	    }
	}
    
	if( ((e-d) > 4) || (((e-d) == 4) && (*d & 0x70)) ) {
	    
	} else {
	    PRUint32 n = 0;
      
	    switch( e-d ) {
	    case 4:
		n |= ((PRUint32)(e[-4] & 0x0f)) << 28;
	    case 3:
		n |= ((PRUint32)(e[-3] & 0x7f)) << 21;
	    case 2:
		n |= ((PRUint32)(e[-2] & 0x7f)) << 14;
	    case 1:
		n |= ((PRUint32)(e[-1] & 0x7f)) <<  7;
	    case 0:
		n |= ((PRUint32)(e[-0] & 0x7f))      ;
	    }
      
	    if( (char *)NULL == a ) {
		
		PRUint32 one = PR_MIN(n/40, 2); 
		PRUint32 two = n - one * 40;
        
		a = PR_smprintf("OID.%lu.%lu", one, two);
		if( (char *)NULL == a ) {
		    PORT_SetError(SEC_ERROR_NO_MEMORY);
		    return (char *)NULL;
		}
	    } else {
		b = PR_smprintf("%s.%lu", a, n);
		if( (char *)NULL == b ) {
		    PR_smprintf_free(a);
		    PORT_SetError(SEC_ERROR_NO_MEMORY);
		    return (char *)NULL;
		}
        
		PR_smprintf_free(a);
		a = b;
	    }
	}
    }

    return a;
}


static SECItem *
get_hex_string(SECItem *data)
{
    SECItem *rv;
    unsigned int i, j;
    static const char hex[] = { "0123456789ABCDEF" };

    
    rv = SECITEM_AllocItem(NULL, NULL, data->len*2 + 2);
    if (!rv) {
	return NULL;
    }
    rv->data[0] = '#';
    rv->len = 1 + 2 * data->len;
    for (i=0; i<data->len; i++) {
	j = data->data[i];
	rv->data[2*i+1] = hex[j >> 4];
	rv->data[2*i+2] = hex[j & 15];
    }
    rv->data[rv->len] = 0;
    return rv;
}

























































static SECStatus
AppendAVA(stringBuf *bufp, CERTAVA *ava, CertStrictnessLevel strict)
{
    const NameToKind *pn2k   = name2kinds;
    SECItem     *avaValue    = NULL;
    char        *unknownTag  = NULL;
    char        *encodedAVA  = NULL;
    PRBool       useHex      = PR_FALSE;  
    SECOidTag    endKind;
    SECStatus    rv;
    unsigned int len;
    int          nameLen, valueLen;
    NameToKind   n2k         = { NULL, 32767, SEC_OID_UNKNOWN, SEC_ASN1_DS };
    char         tmpBuf[384];

#define tagName  n2k.name    /* non-NULL means use NAME= form */
#define maxBytes n2k.maxLen
#define tag      n2k.kind
#define vt       n2k.valueType

    



    endKind = (strict == CERT_N2A_READABLE) ? SEC_OID_UNKNOWN
                                            : SEC_OID_AVA_POSTAL_ADDRESS;
    tag = CERT_GetAVATag(ava);
    while (pn2k->kind != tag && pn2k->kind != endKind) {
        ++pn2k;
    }

    if (pn2k->kind != endKind ) {
        n2k = *pn2k;
    } else if (strict != CERT_N2A_READABLE) {
        useHex = PR_TRUE;
    }
    
    if (strict == CERT_N2A_INVERTIBLE && vt == SEC_ASN1_DS) {
	tagName = NULL;      
	useHex = PR_TRUE;    
    }
    if (!useHex) {
	avaValue = CERT_DecodeAVAValue(&ava->value);
	if (!avaValue) {
	    useHex = PR_TRUE;
	    if (strict != CERT_N2A_READABLE) {
		tagName = NULL;  
	    }
	}
    }
    if (!tagName) {
	
	tagName = unknownTag = CERT_GetOidString(&ava->type);
	if (!tagName) {
	    if (avaValue)
		SECITEM_FreeItem(avaValue, PR_TRUE);
	    return SECFailure;
	}
    }
    if (useHex) {
	avaValue = get_hex_string(&ava->value);
	if (!avaValue) {
	    if (unknownTag) 
	    	PR_smprintf_free(unknownTag);
	    return SECFailure;
	}
    }

    if (strict == CERT_N2A_READABLE) {
    	if (maxBytes > sizeof(tmpBuf) - 4)
	    maxBytes = sizeof(tmpBuf) - 4;
	
	if (avaValue->len > maxBytes + 3) {
	    




	    len = maxBytes;
	    while (((avaValue->data[len] & 0xc0) == 0x80) && len > 0) {
	       len--;
	    }
	    
	    avaValue->data[len++] = '.'; 
	    avaValue->data[len++] = '.';
	    avaValue->data[len++] = '.';
	    avaValue->data[len]   = 0;
	    avaValue->len = len;
	}
    }

    nameLen  = strlen(tagName);
    valueLen = (useHex ? avaValue->len : 
            cert_RFC1485_GetRequiredLen(avaValue->data, avaValue->len, NULL));
    len = nameLen + valueLen + 2; 

    if (len <= sizeof(tmpBuf)) {
    	encodedAVA = tmpBuf;
    } else if (strict == CERT_N2A_READABLE) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
    } else {
	encodedAVA = PORT_Alloc(len);
    }
    if (!encodedAVA) {
	SECITEM_FreeItem(avaValue, PR_TRUE);
	if (unknownTag) 
	    PR_smprintf_free(unknownTag);
	return SECFailure;
    }
    memcpy(encodedAVA, tagName, nameLen);
    if (unknownTag) 
    	PR_smprintf_free(unknownTag);
    encodedAVA[nameLen++] = '=';
    
    
    if (useHex) {
	memcpy(encodedAVA + nameLen, (char *)avaValue->data, avaValue->len);
	encodedAVA[nameLen + avaValue->len] = '\0';
	rv = SECSuccess;
    } else 
	rv = CERT_RFC1485_EscapeAndQuote(encodedAVA + nameLen, len - nameLen, 
		    		        (char *)avaValue->data, avaValue->len);
    SECITEM_FreeItem(avaValue, PR_TRUE);
    if (rv == SECSuccess)
	rv = AppendStr(bufp, encodedAVA);
    if (encodedAVA != tmpBuf)
    	PORT_Free(encodedAVA);
    return rv;
}

#undef tagName
#undef maxBytes
#undef tag
#undef vt

char *
CERT_NameToAsciiInvertible(CERTName *name, CertStrictnessLevel strict)
{
    CERTRDN** rdns;
    CERTRDN** lastRdn;
    CERTRDN** rdn;
    PRBool first = PR_TRUE;
    stringBuf strBuf = { NULL, 0, 0 };
    
    rdns = name->rdns;
    if (rdns == NULL) {
	return NULL;
    }
    
    
    lastRdn = rdns;
    while (*lastRdn) lastRdn++;
    lastRdn--;
    
    


    for (rdn = lastRdn; rdn >= rdns; rdn--) {
	CERTAVA** avas = (*rdn)->avas;
	CERTAVA* ava;
	PRBool newRDN = PR_TRUE;

	


	while (avas && (ava = *avas++) != NULL) {
	    SECStatus rv;
	    
	    if (!first) {
		
		rv = AppendStr(&strBuf, newRDN ? "," : "+");
		if (rv) goto loser;
	    } else {
		first = PR_FALSE;
	    }
	    
	    
	    rv = AppendAVA(&strBuf, ava, strict);
	    if (rv) goto loser;
	    newRDN = PR_FALSE;
	}
    }
    return strBuf.buffer;
loser:
    if (strBuf.buffer) {
	PORT_Free(strBuf.buffer);
    }
    return NULL;
}

char *
CERT_NameToAscii(CERTName *name)
{
    return CERT_NameToAsciiInvertible(name, CERT_N2A_READABLE);
}





char *
CERT_DerNameToAscii(SECItem *dername)
{
    int rv;
    PRArenaPool *arena = NULL;
    CERTName name;
    char *retstr = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( arena == NULL) {
	goto loser;
    }
    
    rv = SEC_QuickDERDecodeItem(arena, &name, CERT_NameTemplate, dername);
    
    if ( rv != SECSuccess ) {
	goto loser;
    }

    retstr = CERT_NameToAscii(&name);

loser:
    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(retstr);
}




static char *
CERT_GetNameElement(PRArenaPool *arena, CERTName *name, int wantedTag)
{
    CERTRDN** rdns;
    CERTRDN *rdn;
    char *buf = 0;
    
    rdns = name->rdns;
    while (rdns && (rdn = *rdns++) != 0) {
	CERTAVA** avas = rdn->avas;
	CERTAVA*  ava;
	while (avas && (ava = *avas++) != 0) {
	    int tag = CERT_GetAVATag(ava);
	    if ( tag == wantedTag ) {
		SECItem *decodeItem = CERT_DecodeAVAValue(&ava->value);
		if(!decodeItem) {
		    return NULL;
		}
		if (arena) {
		    buf = (char *)PORT_ArenaZAlloc(arena,decodeItem->len + 1);
		} else {
		    buf = (char *)PORT_ZAlloc(decodeItem->len + 1);
		}
		if ( buf ) {
		    PORT_Memcpy(buf, decodeItem->data, decodeItem->len);
		    buf[decodeItem->len] = 0;
		}
		SECITEM_FreeItem(decodeItem, PR_TRUE);
		goto done;
	    }
	}
    }
    
  done:
    return buf;
}





static char *
CERT_GetLastNameElement(PRArenaPool *arena, CERTName *name, int wantedTag)
{
    CERTRDN** rdns;
    CERTRDN *rdn;
    CERTAVA * lastAva = NULL;
    char *buf = 0;
    
    rdns = name->rdns;
    while (rdns && (rdn = *rdns++) != 0) {
	CERTAVA** avas = rdn->avas;
	CERTAVA*  ava;
	while (avas && (ava = *avas++) != 0) {
	    int tag = CERT_GetAVATag(ava);
	    if ( tag == wantedTag ) {
		lastAva = ava;
	    }
	}
    }

    if (lastAva) {
	SECItem *decodeItem = CERT_DecodeAVAValue(&lastAva->value);
	if(!decodeItem) {
	    return NULL;
	}
	if (arena) {
	    buf = (char *)PORT_ArenaZAlloc(arena,decodeItem->len + 1);
	} else {
	    buf = (char *)PORT_ZAlloc(decodeItem->len + 1);
	}
	if ( buf ) {
	    PORT_Memcpy(buf, decodeItem->data, decodeItem->len);
	    buf[decodeItem->len] = 0;
	}
	SECITEM_FreeItem(decodeItem, PR_TRUE);
    }    
    return buf;
}

char *
CERT_GetCertificateEmailAddress(CERTCertificate *cert)
{
    char *rawEmailAddr = NULL;
    SECItem subAltName;
    SECStatus rv;
    CERTGeneralName *nameList = NULL;
    CERTGeneralName *current;
    PRArenaPool *arena = NULL;
    int i;
    
    subAltName.data = NULL;

    rawEmailAddr = CERT_GetNameElement(cert->arena, &(cert->subject),
						 SEC_OID_PKCS9_EMAIL_ADDRESS);
    if ( rawEmailAddr == NULL ) {
	rawEmailAddr = CERT_GetNameElement(cert->arena, &(cert->subject), 
							SEC_OID_RFC1274_MAIL);
    }
    if ( rawEmailAddr == NULL) {

	rv = CERT_FindCertExtension(cert,  SEC_OID_X509_SUBJECT_ALT_NAME, 
								&subAltName);
	if (rv != SECSuccess) {
	    goto finish;
	}
	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (!arena) {
	    goto finish;
	}
	nameList = current = CERT_DecodeAltNameExtension(arena, &subAltName);
	if (!nameList ) {
	    goto finish;
	}
	if (nameList != NULL) {
	    do {
		if (current->type == certDirectoryName) {
		    rawEmailAddr = CERT_GetNameElement(cert->arena,
			&(current->name.directoryName), 
					       SEC_OID_PKCS9_EMAIL_ADDRESS);
		    if ( rawEmailAddr == NULL ) {
			rawEmailAddr = CERT_GetNameElement(cert->arena,
			  &(current->name.directoryName), SEC_OID_RFC1274_MAIL);
		    }
		} else if (current->type == certRFC822Name) {
		    rawEmailAddr = (char*)PORT_ArenaZAlloc(cert->arena,
						current->name.other.len + 1);
		    if (!rawEmailAddr) {
			goto finish;
		    }
		    PORT_Memcpy(rawEmailAddr, current->name.other.data, 
				current->name.other.len);
		    rawEmailAddr[current->name.other.len] = '\0';
		}
		if (rawEmailAddr) {
		    break;
		}
		current = CERT_GetNextGeneralName(current);
	    } while (current != nameList);
	}
    }
    if (rawEmailAddr) {
	for (i = 0; i <= (int) PORT_Strlen(rawEmailAddr); i++) {
	    rawEmailAddr[i] = tolower(rawEmailAddr[i]);
	}
    } 

finish:

    

    if (arena) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    if ( subAltName.data ) {
	SECITEM_FreeItem(&subAltName, PR_FALSE);
    }

    return(rawEmailAddr);
}

static char *
appendStringToBuf(char *dest, char *src, PRUint32 *pRemaining)
{
    PRUint32 len;
    if (dest && src && src[0] && *pRemaining > (len = PL_strlen(src))) {
	PRUint32 i;
	for (i = 0; i < len; ++i)
	    dest[i] = tolower(src[i]);
	dest[len] = 0;
	dest        += len + 1;
	*pRemaining -= len + 1;
    }
    return dest;
}

static char *
appendItemToBuf(char *dest, SECItem *src, PRUint32 *pRemaining)
{
    if (dest && src && src->data && src->len && src->data[0] && 
        *pRemaining > src->len + 1 ) {
	PRUint32 len = src->len;
	PRUint32 i;
	for (i = 0; i < len && src->data[i] ; ++i)
	    dest[i] = tolower(src->data[i]);
	dest[len] = 0;
	dest        += len + 1;
	*pRemaining -= len + 1;
    }
    return dest;
}





char *
cert_GetCertificateEmailAddresses(CERTCertificate *cert)
{
    char *           rawEmailAddr = NULL;
    char *           addrBuf      = NULL;
    char *           pBuf         = NULL;
    PRArenaPool *    tmpArena     = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    PRUint32         maxLen       = 0;
    PRInt32          finalLen     = 0;
    SECStatus        rv;
    SECItem          subAltName;
    
    if (!tmpArena) 
    	return addrBuf;

    subAltName.data = NULL;
    maxLen = cert->derCert.len;
    PORT_Assert(maxLen);
    if (!maxLen) 
	maxLen = 2000;  

    pBuf = addrBuf = (char *)PORT_ArenaZAlloc(tmpArena, maxLen + 1);
    if (!addrBuf) 
    	goto loser;

    rawEmailAddr = CERT_GetNameElement(tmpArena, &cert->subject,
				       SEC_OID_PKCS9_EMAIL_ADDRESS);
    pBuf = appendStringToBuf(pBuf, rawEmailAddr, &maxLen);

    rawEmailAddr = CERT_GetNameElement(tmpArena, &cert->subject, 
				       SEC_OID_RFC1274_MAIL);
    pBuf = appendStringToBuf(pBuf, rawEmailAddr, &maxLen);

    rv = CERT_FindCertExtension(cert,  SEC_OID_X509_SUBJECT_ALT_NAME, 
				&subAltName);
    if (rv == SECSuccess && subAltName.data) {
	CERTGeneralName *nameList     = NULL;

	if (!!(nameList = CERT_DecodeAltNameExtension(tmpArena, &subAltName))) {
	    CERTGeneralName *current = nameList;
	    do {
		if (current->type == certDirectoryName) {
		    rawEmailAddr = CERT_GetNameElement(tmpArena,
			                       &current->name.directoryName, 
					       SEC_OID_PKCS9_EMAIL_ADDRESS);
		    pBuf = appendStringToBuf(pBuf, rawEmailAddr, &maxLen);

		    rawEmailAddr = CERT_GetNameElement(tmpArena,
					      &current->name.directoryName, 
					      SEC_OID_RFC1274_MAIL);
		    pBuf = appendStringToBuf(pBuf, rawEmailAddr, &maxLen);
		} else if (current->type == certRFC822Name) {
		    pBuf = appendItemToBuf(pBuf, &current->name.other, &maxLen);
		}
		current = CERT_GetNextGeneralName(current);
	    } while (current != nameList);
	}
	SECITEM_FreeItem(&subAltName, PR_FALSE);
	
    }
    
    finalLen = (pBuf - addrBuf) + 1;
    pBuf = NULL;
    if (finalLen > 1) {
	pBuf = PORT_ArenaAlloc(cert->arena, finalLen);
	if (pBuf) {
	    PORT_Memcpy(pBuf, addrBuf, finalLen);
	}
    }
loser:
    if (tmpArena)
	PORT_FreeArena(tmpArena, PR_FALSE);

    return pBuf;
}





const char *	
CERT_GetFirstEmailAddress(CERTCertificate * cert)
{
    if (cert && cert->emailAddr && cert->emailAddr[0])
    	return (const char *)cert->emailAddr;
    return NULL;
}





const char *	
CERT_GetNextEmailAddress(CERTCertificate * cert, const char * prev)
{
    if (cert && prev && prev[0]) {
    	PRUint32 len = PL_strlen(prev);
	prev += len + 1;
	if (prev && prev[0])
	    return prev;
    }
    return NULL;
}





char *
CERT_GetCertEmailAddress(CERTName *name)
{
    char *rawEmailAddr;
    char *emailAddr;

    
    rawEmailAddr = CERT_GetNameElement(NULL, name, SEC_OID_PKCS9_EMAIL_ADDRESS);
    if ( rawEmailAddr == NULL ) {
	rawEmailAddr = CERT_GetNameElement(NULL, name, SEC_OID_RFC1274_MAIL);
    }
    emailAddr = CERT_FixupEmailAddr(rawEmailAddr);
    if ( rawEmailAddr ) {
	PORT_Free(rawEmailAddr);
    }
    return(emailAddr);
}


char *
CERT_GetCommonName(CERTName *name)
{
    return(CERT_GetLastNameElement(NULL, name, SEC_OID_AVA_COMMON_NAME));
}

char *
CERT_GetCountryName(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_AVA_COUNTRY_NAME));
}

char *
CERT_GetLocalityName(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_AVA_LOCALITY));
}

char *
CERT_GetStateName(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_AVA_STATE_OR_PROVINCE));
}

char *
CERT_GetOrgName(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_AVA_ORGANIZATION_NAME));
}

char *
CERT_GetDomainComponentName(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_AVA_DC));
}

char *
CERT_GetOrgUnitName(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME));
}

char *
CERT_GetDnQualifier(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_AVA_DN_QUALIFIER));
}

char *
CERT_GetCertUid(CERTName *name)
{
    return(CERT_GetNameElement(NULL, name, SEC_OID_RFC1274_UID));
}

