






































#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"

#define MKI_BUF_LEN 4

sdp_mca_t *sdp_alloc_mca () {
    sdp_mca_t           *mca_p;

    
    mca_p = (sdp_mca_t *)SDP_MALLOC(sizeof(sdp_mca_t));
    if (mca_p == NULL) {
        return (NULL);
    }
    
    mca_p->media              = SDP_MEDIA_INVALID;
    mca_p->conn.nettype       = SDP_NT_INVALID;
    mca_p->conn.addrtype      = SDP_AT_INVALID;
    mca_p->conn.conn_addr[0]  = '\0';
    mca_p->conn.is_multicast  = FALSE;
    mca_p->conn.ttl           = 0;
    mca_p->conn.num_of_addresses = 0;
    mca_p->transport          = SDP_TRANSPORT_INVALID;
    mca_p->port               = SDP_INVALID_VALUE;
    mca_p->num_ports          = SDP_INVALID_VALUE;
    mca_p->vpi                = SDP_INVALID_VALUE;
    mca_p->vci                = 0;
    mca_p->vcci               = SDP_INVALID_VALUE;
    mca_p->cid                = SDP_INVALID_VALUE;
    mca_p->num_payloads       = 0;
    mca_p->sessinfo_found     = FALSE;
    mca_p->encrypt.encrypt_type  = SDP_ENCRYPT_INVALID;
    mca_p->media_attrs_p      = NULL;
    mca_p->next_p             = NULL;
    mca_p->mid                = 0;
    mca_p->bw.bw_data_count   = 0;
    mca_p->bw.bw_data_list    = NULL;

    return (mca_p);
}























tinybool
verify_sdescriptions_mki (char *buf, char *mkiVal, u16 *mkiLen)
{

    char       *ptr,
               mkiValBuf[SDP_SRTP_MAX_MKI_SIZE_BYTES],
	       mkiLenBuf[MKI_BUF_LEN];
    int        idx = 0;
    
    ptr = buf;
    
    if (!ptr || (!isdigit((int) *ptr))) {
        return FALSE;
    }
   
    
    while (*ptr) {
        if (*ptr == ':') {
	    
	    mkiValBuf[idx] = 0;
	    ptr++;
	    break;
	} else if ((isdigit((int) *ptr) && (idx < SDP_SRTP_MAX_MKI_SIZE_BYTES-1))) {
	     mkiValBuf[idx++] = *ptr;
	} else {
	     return FALSE;
	}
	   
	ptr++;
    }
    
    
    if (*ptr == 0) {
        return FALSE;
    } 
	
    idx = 0;
    
    
    while (*ptr) {
        if (isdigit((int) *ptr) && (idx < 3)) {
	    mkiLenBuf[idx++] = *ptr;
	} else {
	    return FALSE;
	}
	
	ptr++;
    }
    
    mkiLenBuf[idx] = 0;
    *mkiLen = atoi(mkiLenBuf);
    
    
    if (*mkiLen > 0 && *mkiLen <= 128) {
	 strncpy(mkiVal, mkiValBuf, MKI_BUF_LEN);
         return TRUE;
    } else {
         return FALSE;
    }
    
}














 
tinybool
verify_sdescriptions_lifetime (char *buf)
{

    char     *ptr;
    tinybool tokenFound = FALSE;
	    
    ptr = buf;
    if (!ptr || *ptr == 0) {
        return FALSE;
    }
    
    while (*ptr) {
        if (*ptr == '^') {
	    if (tokenFound) {
	        
                return FALSE;
            } else {
                tokenFound = TRUE;
                


		 
                if (buf[0] != '2' || buf[1] != '^') {
		    return FALSE;
                }
            }
        } else if (!isdigit((int) *ptr)) {
	           return FALSE;
        }
    
        ptr++;
	
    }
    
    
    if (tokenFound) {
        if (strlen(buf) <= 2) {
	    return FALSE;
	}
    }
    
    return TRUE;
}








tinybool
sdp_validate_maxprate(const char *string_parm)
{
    tinybool retval = FALSE;

    if (string_parm && (*string_parm)) {
        while (isdigit((int)*string_parm)) {
            string_parm++;
        }

        if (*string_parm == '.') {
            string_parm++;
            while (isdigit((int)*string_parm)) {
                string_parm++;
            }
        } 

        if (*string_parm == '\0') {
            retval = TRUE;
        } else {
            retval = FALSE;
        }
    }

    return retval;
}

char *sdp_findchar (const char *ptr, char *char_list)
{
    int i;

    for (;*ptr != '\0'; ptr++) {
	for (i=0; char_list[i] != '\0'; i++) {
	    if (*ptr == char_list[i]) {
		return ((char *)ptr);
	    }
	}
    }
    return ((char *)ptr);
}





char *sdp_getnextstrtok (const char *str, char *tokenstr, 
                         char *delim, sdp_result_e *result)
{
    char *b, *tmp;
    int   flag2moveon;

    if ((str == NULL) || (tokenstr == NULL)) {
        *result = SDP_FAILURE;
        return( (char *)str );
    }

    
    for ( ; ((*str != '\0') && (*str != '\n') && (*str != '\r')); str++) {
        flag2moveon = 1;  
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 0;
            }
        }
        if( flag2moveon ) {
            break;  
        }
    }
   
    
    if ((*str == '\0') || (*str == '\n') || (*str == '\r')) {
        *result = SDP_FAILURE;
        return( (char *)str );
    }

    
    flag2moveon = 0;
    tmp = tokenstr;

    while (((tokenstr-tmp) < SDP_MAX_STRING_LEN) &&
           (*str != '\0') && (*str != '\n') && (*str != '\r')) {
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 1;
                break;
            }
        }

        if( flag2moveon ) {
            break;
        } else {
            *tokenstr++ = *str++;
        }
    }

    *tokenstr = '\0';     

    *result = SDP_SUCCESS;
    return((char *)str);
}








u32 sdp_getnextnumtok_or_null (const char *str, const char **str_end, 
                               char *delim, tinybool *null_ind,
                               sdp_result_e *result)
{
    char *b;
    char  tmp[SDP_MAX_STRING_LEN+1];
    char *tok = tmp;
    u32   numval=0;
    int   flag2moveon;

    if ((str == NULL)  || (str_end == NULL)) {
        *result = SDP_FAILURE;
        return(numval);
    }

    
    for ( ; ((*str != '\0') && (*str != '\n') && (*str != '\r')); str++) {
        flag2moveon = 1;  
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 0;
            }
        }
        if( flag2moveon ) {
            break;  
        }
    }

    
    if ((*str == '\0') || (*str == '\n') || (*str == '\r')) {
        *result = SDP_FAILURE;
        *str_end = (char *)str;
        return(numval);
    }

    
    flag2moveon = 0;

    while (((tok-tmp) < SDP_MAX_STRING_LEN) &&
           (*str != '\0') && (*str != '\n') && (*str != '\r')) {
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 1;
                break;
            }
        }
        if( flag2moveon ) {
            break;
        } else {
            *tok++ = *str++;
        }
    }

    *tok = '\0';     

    
    if (tmp[0] == '-') {
        *null_ind = TRUE;
        *result = SDP_SUCCESS;
        *str_end = (char *)str;
        return (0);
    } else {
        *null_ind = FALSE;
    }

    
    for (tok = tmp; *tok != '\0'; tok++) {
        if (!(*tok >= '0' && *tok <= '9')) {
            *result = SDP_FAILURE;
            *str_end = (char *)str;
            return(numval);
        } else {
            numval = (numval * 10) + (*tok - '0');
        }
    }

    *result = SDP_SUCCESS;
    *str_end = (char *)str;
    return(numval);
}






u32 sdp_getnextnumtok (const char *str, const char **str_end, 
                       char *delim, sdp_result_e *result)
{
    char *b;
    char  tmp[SDP_MAX_STRING_LEN+1];
    char *tok = tmp;
    u32   numval=0;
    int   flag2moveon;

    if ((str == NULL)  || (str_end == NULL)) {
        *result = SDP_FAILURE;
        return(numval);
    }

    
    for ( ; ((*str != '\0') && (*str != '\n') && (*str != '\r')); str++) {
        flag2moveon = 1;  
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 0;
            }
        }
        if( flag2moveon ) {
            break;  
        }
    }

    
    if ((*str == '\0') || (*str == '\n') || (*str == '\r')) {
        *result = SDP_FAILURE;
        *str_end = (char *)str;
        return(numval);
    }

    
    flag2moveon = 0;

    while (((tok-tmp) < SDP_MAX_STRING_LEN) &&
           (*str != '\0') && (*str != '\n') && (*str != '\r')) {
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 1;
                break;
            }
        }
        if( flag2moveon ) {
            break;
        } else {
            *tok++ = *str++;
        }
    }

    *tok = '\0';     

    
    for (tok = tmp; *tok != '\0'; tok++) {
        if (!(*tok >= '0' && *tok <= '9')) {
            *result = SDP_FAILURE;
            *str_end = (char *)str;
            return(numval);
        } else {
            numval = (numval * 10) + (*tok - '0');
        }
    }

    *result = SDP_SUCCESS;
    *str_end = (char *)str;
    return(numval);
}







tinybool sdp_getchoosetok (const char *str, char **str_end, 
                           char *delim, sdp_result_e *result)
{
    char *b;
    int   flag2moveon;

    if ((str == NULL)  || (str_end == NULL)) {
        *result = SDP_FAILURE;
        return(FALSE);
    }

    
    for ( ; ((*str != '\0') && (*str != '\n') && (*str != '\r')); str++) {
        flag2moveon = 1;  
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 0;
            }
        }
        if( flag2moveon ) {
            break;  
        }
    }

    
    if ((*str == '\0') || (*str == '\n') || (*str == '\r')) {
        *result = SDP_FAILURE;
        *str_end = (char *)str;
        return(FALSE);
    }

    
    if (*str == '$') {
        str++;
        if ((*str == '\0') || (*str == '\n') || (*str == '\r')) {
            *result = SDP_SUCCESS;
            
            *str_end = (char *)(str+1);
            return(TRUE);
        }
        for (b=delim; *b; b++) {
            if (*str == *b) {
                *result = SDP_SUCCESS;
                
                *str_end = (char *)(str+1);
                return(TRUE);
            }
        }
    }

    
    *result = SDP_SUCCESS;
    *str_end = (char *)str;
    return(FALSE);

}







char *sdp_getnextstrtok_noskip (const char *str, char *tokenstr, 
                         char *delim, sdp_result_e *result)
{
    char *b, *tmp;
    int   flag2moveon, delimiter_count;

    if ((str == NULL) || (tokenstr == NULL)) {
        *result = SDP_FAILURE;
        return( (char *)str );
    }

    
    delimiter_count = 0;
    for ( ; ((*str != '\0') && (*str != '\n') && (*str != '\r')); str++) {
        flag2moveon = 1;  
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 0;
		if (++delimiter_count > 1) {
		  *tokenstr = '\0';     
		  return( (char *)str );
		}
            }
        }
        if( flag2moveon ) {
            break;  
        }
    }
   
    
    if ((*str == '\0') || (*str == '\n') || (*str == '\r')) {
        *result = SDP_FAILURE;
        return( (char *)str );
    }

    
    flag2moveon = 0;
    tmp = tokenstr;

    while (((tokenstr-tmp) < SDP_MAX_STRING_LEN) &&
           (*str != '\0') && (*str != '\n') && (*str != '\r')) {
        for (b=delim; *b; b++) {
            if (*str == *b) {
                flag2moveon = 1;
                break;
            }
        }

        if( flag2moveon ) {
            break;
        } else {
            *tokenstr++ = *str++;
        }
    }

    *tokenstr = '\0';     

    *result = SDP_SUCCESS;
    return((char *)str);
}



















 
static const char crypto_string[] = "X-crypto:";
static const int crypto_strlen = sizeof(crypto_string) - 1;
static const char inline_string[] = "inline:";
static const int inline_strlen = sizeof(inline_string) - 1;

static const char star_string[] = "****************************************";
static const int star_strlen = sizeof(star_string) - 1;


















#define MIN_CRYPTO_STRING_SIZE_BYTES 21













#define CHAR_IS_WHITESPACE(_test_char) \
    ((((_test_char)==' ')||((_test_char)=='\t'))?1:0)

#define SKIP_WHITESPACE(_cptr, _max_cptr)	    \
    while ((_cptr)<=(_max_cptr)) {		    \
	if (!CHAR_IS_WHITESPACE(*(_cptr))) break;   \
	(_cptr)++;				    \
    }

#define FIND_WHITESPACE(_cptr, _max_cptr)	    \
    while ((_cptr)<=(_max_cptr)) {		    \
	if (CHAR_IS_WHITESPACE(*(_cptr))) break;    \
	(_cptr)++;				    \
    }












void sdp_crypto_debug (char *buffer, ulong length_bytes)
{
    char *current, *start;
    char *last = buffer + length_bytes;
    int result;

    




    for (start=current=buffer; 
	 current<=last-MIN_CRYPTO_STRING_SIZE_BYTES; 
	 current++) {
	if ((*current == 'x') || (*current == 'X')) {
	    result = cpr_strncasecmp(current, crypto_string, crypto_strlen);
	    if (!result) {
		current += crypto_strlen;
		if (current > last) break;

		
		FIND_WHITESPACE(current, last);

		
		SKIP_WHITESPACE(current, last);

		
		result = cpr_strncasecmp(current, inline_string, inline_strlen);
		if (!result) {
		    int star_count = 0;

		    current += inline_strlen;
		    if (current > last) break;

		    sdp_dump_buffer(start, current - start);

		    
		    while (current<=last) {
			if (*current == '|' || *current == '\n') {
			    
			    while (star_count > star_strlen) {
				



				sdp_dump_buffer((char*)star_string, star_strlen);
				star_count -= star_strlen;
			    }
			    sdp_dump_buffer((char*)star_string, star_count);
			    break;
			} else {
			    star_count++;
			    current++;
			}
		    }
		    
		    start=current;
		}
	    }
	}
    }

    if (last > start) {
	
	sdp_dump_buffer(start, last - start);
    }
}

















char * sdp_debug_msg_filter (char *buffer, ulong length_bytes)
{
    char *current;
    char *last = buffer + length_bytes;
    int result;

    SDP_PRINT("\n%s:%d: Eliding sensitive data from debug output",
	    __FILE__, __LINE__);
    




    for (current=buffer;
	 current<=last-MIN_CRYPTO_STRING_SIZE_BYTES;
	 current++) {
	if ((*current == 'x') || (*current == 'X')) {
	    result = cpr_strncasecmp(current, crypto_string, crypto_strlen);
	    if (!result) {
		current += crypto_strlen;
		if (current > last) break;

		
		FIND_WHITESPACE(current, last);

		
		SKIP_WHITESPACE(current, last);

		
		result = cpr_strncasecmp(current, inline_string, inline_strlen);
		if (!result) {
		    current += inline_strlen;
		    if (current > last) break;

		    
		    while (current<=last) {
			if (*current == '|' || *current == '\n') {
			    
			    break;
			} else {
			    *current = '*';
			    current++;
			}
		    }
		}
	    }
	}
    }

    return buffer;
}














tinybool sdp_checkrange (sdp_t *sdp_p, char *num, ulong *u_val)  
{
    ulong l_val;
    char *endP = NULL;
    *u_val = 0;

    if (!num || !*num) {
        return FALSE;
    }

    if (*num == '-') {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            SDP_ERROR("%s ERROR: Parameter value is a negative number: %s",
                      sdp_p->debug_str, num);
        }
        return FALSE;
    }

    l_val = strtoul(num, &endP, 10);
    if (*endP == '\0') {

        if (l_val > 4294967295UL) {
	    if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
		SDP_ERROR("%s ERROR: Parameter value: %s is greater than 4294967295",
			  sdp_p->debug_str, num);
	    }
	    return FALSE;
	}

	if (l_val == 4294967295UL) {
	    





	    if (strcmp("4294967295", num)) {
		if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
		    SDP_ERROR("%s ERROR: Parameter value: %s is greater than 4294967295",
			      sdp_p->debug_str, num);
		}
		return FALSE;
	    }
	}
    }
    *u_val = l_val;
    return TRUE;
}

#undef CHAR_IS_WHITESPACE
#undef SKIP_WHITESPACE
#undef FIND_WHITESPACE
