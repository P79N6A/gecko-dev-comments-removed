





































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



    { "E",             128, SEC_OID_PKCS9_EMAIL_ADDRESS,SEC_ASN1_IA5_STRING},

#if 0 
    { "pseudonym",      64, SEC_OID_AVA_PSEUDONYM,      SEC_ASN1_DS},
#endif

    { 0,           256, SEC_OID_UNKNOWN                      , 0},
};


static const PRInt16 x2b[256] = {
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

#define IS_HEX(c) (x2b[(PRUint8)(c)] >= 0)

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






#define NEEDS_ESCAPE(c) \
    (c == C_DOUBLE_QUOTE || c == C_BACKSLASH)

#define NEEDS_HEX_ESCAPE(c) \
    ((PRUint8)c < 0x20 || c == 0x7f)

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


static int
scanVal(char **pbp, char *endptr, char *valBuf, int valBufSize)  
{
    char *bp, *valBufp;
    int vallen = 0;
    PRBool isQuoted;
    
    PORT_Assert(valBufSize > 0);
    
    
    skipSpace(pbp, endptr);
    if(*pbp == endptr) {
	
	return 0;
    }
    
    bp = *pbp;
    
    
    if (*bp == C_DOUBLE_QUOTE) {
	isQuoted = PR_TRUE;
	
	bp++;
    } else {
	isQuoted = PR_FALSE;
    }
    
    valBufp = valBuf;
    while (bp < endptr) {
	char c = *bp;
	if (c == C_BACKSLASH) {
	    
	    bp++;
	    if (bp >= endptr) {
		
		*pbp = bp;
		return 0;
	    }
	    c = *bp;
	    if (IS_HEX(c) && (endptr - bp) >= 2 && IS_HEX(bp[1])) {
		bp++;
		c = (char)((x2b[(PRUint8)c] << 4) | x2b[(PRUint8)*bp]); 
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
	    return 0;
	}
	*valBufp++ = c;
	bp++;
    }
    
    
    if (!isQuoted) {
	while (valBufp > valBuf) {
	    char c = valBufp[-1];
	    if (! OPTIONAL_SPACE(c))
	        break;
	    --valBufp;
	}
	vallen = valBufp - valBuf;
    }
    
    if (isQuoted) {
	
	if (*bp != C_DOUBLE_QUOTE) {
	    *pbp = bp;
	    return 0;
	}
	
	bp++;
	skipSpace(&bp, endptr);
    }
    
    *pbp = bp;
    
    
    *valBufp = 0;
    
    return vallen;
}


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
    SECItem   derVal = { 0, NULL, 0};
    char      sep   = 0;

    char tagBuf[32];
    char valBuf[384];

    PORT_Assert(arena);
    if (SECSuccess != scanTag(pbp, endptr, tagBuf, sizeof tagBuf) ||
	!(valLen    = scanVal(pbp, endptr, valBuf, sizeof valBuf))) {
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
    	
	rv = hexToBin(arena, &derVal, valBuf + 1, valLen - 1);
	if (rv)
	    goto loser;
	a = CERT_CreateAVAFromRaw(arena, &derOid, &derVal);
    } else {
	if (kind == SEC_OID_UNKNOWN)
	    goto loser;
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

	derVal.data = valBuf;
	derVal.len  = valLen;
	a = CERT_CreateAVAFromSECItem(arena, kind, vt, &derVal);
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

typedef enum {
    minimalEscape = 0,		
    minimalEscapeAndQuote,	
    fullEscape                  
} EQMode;










static int
cert_RFC1485_GetRequiredLen(const char *src, int srclen, EQMode *pEQMode)
{
    int i, reqLen=0;
    EQMode mode = pEQMode ? *pEQMode : minimalEscape;
    PRBool needsQuoting = PR_FALSE;
    char lastC = 0;

    
    for (i = 0; i < srclen; i++) {
	char c = src[i];
	reqLen++;
	if (NEEDS_HEX_ESCAPE(c)) {      
	    reqLen += 2;
	} else if (NEEDS_ESCAPE(c)) {   
	    reqLen++;
	} else if (SPECIAL_CHAR(c)) {
	    if (mode == minimalEscapeAndQuote) 
		needsQuoting = PR_TRUE; 
	    else if (mode == fullEscape)
	    	reqLen++;               
	} else if (OPTIONAL_SPACE(c) && OPTIONAL_SPACE(lastC)) {
	    if (mode == minimalEscapeAndQuote) 
		needsQuoting = PR_TRUE; 
	}
	lastC = c;
    }
    
    if (!needsQuoting && srclen > 0 && mode == minimalEscapeAndQuote && 
	(OPTIONAL_SPACE(src[srclen-1]) || OPTIONAL_SPACE(src[0]))) {
	needsQuoting = PR_TRUE;
    }

    if (needsQuoting) 
    	reqLen += 2;
    if (pEQMode && mode == minimalEscapeAndQuote && !needsQuoting)
    	*pEQMode = minimalEscape;
    return reqLen;
}

static const char hexChars[16] = { "0123456789abcdef" };

static SECStatus
escapeAndQuote(char *dst, int dstlen, char *src, int srclen, EQMode *pEQMode)
{
    int i, reqLen=0;
    EQMode mode = pEQMode ? *pEQMode : minimalEscape;

    
    reqLen = cert_RFC1485_GetRequiredLen(src, srclen, &mode) + 1;
    if (reqLen > dstlen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }

    if (mode == minimalEscapeAndQuote)
        *dst++ = C_DOUBLE_QUOTE;
    for (i = 0; i < srclen; i++) {
	char c = src[i];
	if (NEEDS_HEX_ESCAPE(c)) {
	    *dst++ = C_BACKSLASH;
	    *dst++ = hexChars[ (c >> 4) & 0x0f ];
	    *dst++ = hexChars[  c       & 0x0f ];
	} else {
	    if (NEEDS_ESCAPE(c) || (SPECIAL_CHAR(c) && mode == fullEscape)) {
		*dst++ = C_BACKSLASH;
	    }
	    *dst++ = c;
	}
    }
    if (mode == minimalEscapeAndQuote)
    	*dst++ = C_DOUBLE_QUOTE;
    *dst++ = 0;
    if (pEQMode)
    	*pEQMode = mode;
    return SECSuccess;
}

SECStatus
CERT_RFC1485_EscapeAndQuote(char *dst, int dstlen, char *src, int srclen)
{
    EQMode mode = minimalEscapeAndQuote;
    return escapeAndQuote(dst, dstlen, src, srclen, &mode);
}




char *
CERT_GetOidString(const SECItem *oid)
{
    PRUint8 *stop;   
    PRUint8 *first;  
    PRUint8 *last;   
    char *rvString   = NULL;
    char *prefix     = NULL;

#define MAX_OID_LEN 1024 /* bytes */

    if (oid->len > MAX_OID_LEN) {
    	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return NULL;
    }

    
    first = (PRUint8 *)oid->data;
    
    stop = &first[ oid->len ];

    


    if ((*first == 0x80) && (2 == oid->len)) {
	
	rvString = PR_smprintf("%lu", (PRUint32)first[1]);
	if (!rvString) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	}
	return rvString;
    }

    for (; first < stop; first = last + 1) {
    	unsigned int bytesBeforeLast;
    
	for (last = first; last < stop; last++) {
	    if (0 == (*last & 0x80)) {
		break;
	    }
	}
	bytesBeforeLast = (unsigned int)(last - first);
	if (bytesBeforeLast <= 3U) {        
	    PRUint32 n = 0;
	    PRUint32 c;

#define CGET(i, m) \
		c  = last[-i] & m; \
		n |= c << (7 * i)

#define CASE(i, m) \
	    case i:                      \
		CGET(i, m);              \
		if (!n) goto unsupported \
		/* fall-through */

	    switch (bytesBeforeLast) {
	    CASE(3, 0x7f);
	    CASE(2, 0x7f);
	    CASE(1, 0x7f);
	    case 0: n |= last[0] & 0x7f;
		break;
	    }
	    if (last[0] & 0x80)
	    	goto unsupported;
      
	    if (!rvString) {
		
		PRUint32 one = PR_MIN(n/40, 2); 
		PRUint32 two = n - (one * 40);
        
		rvString = PR_smprintf("OID.%lu.%lu", one, two);
	    } else {
		prefix = rvString;
		rvString = PR_smprintf("%s.%lu", prefix, n);
	    }
	} else if (bytesBeforeLast <= 9U) { 
	    PRUint64 n = 0;
	    PRUint64 c;

	    switch (bytesBeforeLast) {
	    CASE(9, 0x01);
	    CASE(8, 0x7f);
	    CASE(7, 0x7f);
	    CASE(6, 0x7f);
	    CASE(5, 0x7f);
	    CASE(4, 0x7f);
	    CGET(3, 0x7f);
	    CGET(2, 0x7f);
	    CGET(1, 0x7f);
	    CGET(0, 0x7f);
		break;
	    }
	    if (last[0] & 0x80)
	    	goto unsupported;
      
	    if (!rvString) {
		
		PRUint64 one = PR_MIN(n/40, 2); 
		PRUint64 two = n - (one * 40);
        
		rvString = PR_smprintf("OID.%llu.%llu", one, two);
	    } else {
		prefix = rvString;
		rvString = PR_smprintf("%s.%llu", prefix, n);
	    }
	} else {
	    
unsupported:
	    if (!rvString)
		rvString = PR_smprintf("OID.UNSUPPORTED");
	    else {
		prefix = rvString;
		rvString = PR_smprintf("%s.UNSUPPORTED", prefix);
	    }
	}

	if (prefix) {
	    PR_smprintf_free(prefix);
	    prefix = NULL;
	}
	if (!rvString) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    break;
	}
    }
    return rvString;
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
#define TMPBUF_LEN 384
    const NameToKind *pn2k   = name2kinds;
    SECItem     *avaValue    = NULL;
    char        *unknownTag  = NULL;
    char        *encodedAVA  = NULL;
    PRBool       useHex      = PR_FALSE;  
    PRBool       truncateName  = PR_FALSE;
    PRBool       truncateValue = PR_FALSE;
    SECOidTag    endKind;
    SECStatus    rv;
    unsigned int len;
    unsigned int nameLen, valueLen;
    unsigned int maxName, maxValue;
    EQMode       mode        = minimalEscapeAndQuote;
    NameToKind   n2k         = { NULL, 32767, SEC_OID_UNKNOWN, SEC_ASN1_DS };
    char         tmpBuf[TMPBUF_LEN];

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

    nameLen  = strlen(tagName);
    valueLen = (useHex ? avaValue->len : 
		cert_RFC1485_GetRequiredLen(avaValue->data, avaValue->len, 
					    &mode));
    len = nameLen + valueLen + 2; 

    maxName  = nameLen;
    maxValue = valueLen;
    if (len <= sizeof(tmpBuf)) {
    	encodedAVA = tmpBuf;
    } else if (strict != CERT_N2A_READABLE) {
	encodedAVA = PORT_Alloc(len);
	if (!encodedAVA) {
	    SECITEM_FreeItem(avaValue, PR_TRUE);
	    if (unknownTag) 
		PR_smprintf_free(unknownTag);
	    return SECFailure;
	}
    } else {
	
	unsigned int fair = (sizeof tmpBuf)/2 - 1; 

	if (nameLen < fair) {
	    
	    maxValue = (sizeof tmpBuf) - (nameLen + 6); 

	} else if (valueLen < fair) {
	    
	    maxName  = (sizeof tmpBuf) - (valueLen + 5); 
	} else {
	    
	    maxName = maxValue = fair - 3;  
	}
	if (nameLen > maxName) {
	    PORT_Assert(unknownTag && unknownTag == tagName);
	    truncateName = PR_TRUE;
	    nameLen = maxName;
	}
    	encodedAVA = tmpBuf;
    }

    memcpy(encodedAVA, tagName, nameLen);
    if (truncateName) {
	


	encodedAVA[nameLen-1] = '.';
	encodedAVA[nameLen-2] = '.';
	encodedAVA[nameLen-3] = '.';
    }
    encodedAVA[nameLen++] = '=';
    if (unknownTag) 
    	PR_smprintf_free(unknownTag);

    if (strict == CERT_N2A_READABLE && maxValue > maxBytes)
	maxValue = maxBytes;
    if (valueLen > maxValue) {
    	valueLen = maxValue;
	truncateValue = PR_TRUE;
    }
    
    if (useHex) {
	char * end = encodedAVA + nameLen + valueLen;
	memcpy(encodedAVA + nameLen, (char *)avaValue->data, valueLen);
	end[0] = '\0';
	if (truncateValue) {
	    end[-1] = '.';
	    end[-2] = '.';
	    end[-3] = '.';
	}
	rv = SECSuccess;
    } else if (!truncateValue) {
	rv = escapeAndQuote(encodedAVA + nameLen, len - nameLen, 
			    (char *)avaValue->data, avaValue->len, &mode);
    } else {
	
	char bigTmpBuf[TMPBUF_LEN * 3 + 3];
	rv = escapeAndQuote(bigTmpBuf, sizeof bigTmpBuf,
			    (char *)avaValue->data, valueLen, &mode);

	bigTmpBuf[valueLen--] = '\0'; 
	
	while (((bigTmpBuf[valueLen] & 0xc0) == 0x80) && valueLen > 0) {
	    bigTmpBuf[valueLen--] = '\0';
	}
	
	bigTmpBuf[++valueLen] = '.';
	bigTmpBuf[++valueLen] = '.';
	bigTmpBuf[++valueLen] = '.';
	if (bigTmpBuf[0] == '"')
	    bigTmpBuf[++valueLen] = '"';
	bigTmpBuf[++valueLen] = '\0';
	PORT_Assert(nameLen + valueLen <= (sizeof tmpBuf) - 1);
	memcpy(encodedAVA + nameLen, bigTmpBuf, valueLen+1);
    }

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
avaToString(PRArenaPool *arena, CERTAVA *ava)
{
    char *    buf       = NULL;
    SECItem*  avaValue;
    int       valueLen;

    avaValue = CERT_DecodeAVAValue(&ava->value);
    if(!avaValue) {
	return buf;
    }
    valueLen = cert_RFC1485_GetRequiredLen(avaValue->data, avaValue->len, 
					   NULL) + 1;
    if (arena) {
	buf = (char *)PORT_ArenaZAlloc(arena, valueLen);
    } else {
	buf = (char *)PORT_ZAlloc(valueLen);
    }
    if (buf) {
	SECStatus rv = escapeAndQuote(buf, valueLen, (char *)avaValue->data, 
	                              avaValue->len, NULL);
	if (rv != SECSuccess) {
	    if (!arena)
		PORT_Free(buf);
	    buf = NULL;
	}
    }
    SECITEM_FreeItem(avaValue, PR_TRUE);
    return buf;
}




static char *
CERT_GetNameElement(PRArenaPool *arena, CERTName *name, int wantedTag)
{
    CERTRDN** rdns = name->rdns;
    CERTRDN*  rdn;
    CERTAVA*  ava  = NULL;

    while (rdns && (rdn = *rdns++) != 0) {
	CERTAVA** avas = rdn->avas;
	while (avas && (ava = *avas++) != 0) {
	    int tag = CERT_GetAVATag(ava);
	    if ( tag == wantedTag ) {
		avas = NULL;
		rdns = NULL; 
	    }
	}
    }
    return ava ? avaToString(arena, ava) : NULL;
}





static char *
CERT_GetLastNameElement(PRArenaPool *arena, CERTName *name, int wantedTag)
{
    CERTRDN** rdns    = name->rdns;
    CERTRDN*  rdn;
    CERTAVA*  lastAva = NULL;
    
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
    return lastAva ? avaToString(arena, lastAva) : NULL;
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

#undef NEEDS_HEX_ESCAPE
#define NEEDS_HEX_ESCAPE(c) (c < 0x20)

static char *
appendItemToBuf(char *dest, SECItem *src, PRUint32 *pRemaining)
{
    if (dest && src && src->data && src->len && src->data[0]) {
	PRUint32 len = src->len;
	PRUint32 i;
	PRUint32 reqLen = len + 1;
	
	for (i = 0; i < len; i++) {
	    if (NEEDS_HEX_ESCAPE(src->data[i]))
	    	reqLen += 2;   
	}
	if (*pRemaining > reqLen) {
	    for (i = 0; i < len; ++i) {
		PRUint8 c = src->data[i];
		if (NEEDS_HEX_ESCAPE(c)) {
		    *dest++ = C_BACKSLASH;
		    *dest++ = hexChars[ (c >> 4) & 0x0f ];
		    *dest++ = hexChars[  c       & 0x0f ];
		} else {
		    *dest++ = tolower(c);
	    	}
	    }
	    *dest++ = '\0';
	    *pRemaining -= reqLen;
	}
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

