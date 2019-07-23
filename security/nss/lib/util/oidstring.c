


































#include <string.h>
#include "secitem.h"
#include "secport.h"
#include "secerr.h"















SECStatus
SEC_StringToOID(PLArenaPool *pool, SECItem *to, const char *from, PRUint32 len)
{
    PRUint32 decimal_numbers = 0;
    PRUint32 result_bytes = 0;
    SECStatus rv;
    PRUint8 result[1024];

    static const PRUint32 max_decimal = (0xffffffff / 10);
    static const char OIDstring[] = {"OID."};

    if (!from || !to) {
    	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    if (!len) {
    	len = PL_strlen(from);
    }
    if (len >= 4 && !PL_strncasecmp(from, OIDstring, 4)) {
    	from += 4; 
	len  -= 4;
    }
    if (!len) {
bad_data:
    	PORT_SetError(SEC_ERROR_BAD_DATA);
	return SECFailure;
    }
    do {
	PRUint32 decimal = 0;
        while (len > 0 && isdigit(*from)) {
	    PRUint32 addend = (*from++ - '0');
	    --len;
	    if (decimal > max_decimal)  
		goto bad_data;
	    decimal = (decimal * 10) + addend;
	    if (decimal < addend)	
		goto bad_data;
	}
	if (len != 0 && *from != '.') {
	    goto bad_data;
	}
	if (decimal_numbers == 0) {
	    if (decimal > 2)
	    	goto bad_data;
	    result[0] = decimal * 40;
	    result_bytes = 1;
	} else if (decimal_numbers == 1) {
	    if (decimal > 40)
	    	goto bad_data;
	    result[0] += decimal;
	} else {
	    
	    PRUint8 * rp;
	    PRUint32 num_bytes = 0;
	    PRUint32 tmp = decimal;
	    while (tmp) {
	        num_bytes++;
		tmp >>= 7;
	    }
	    if (!num_bytes )
	    	++num_bytes;  
	    if (num_bytes + result_bytes > sizeof result)
	    	goto bad_data;
	    tmp = num_bytes;
	    rp = result + result_bytes - 1;
	    rp[tmp] = (PRUint8)(decimal & 0x7f);
	    decimal >>= 7;
	    while (--tmp > 0) {
		rp[tmp] = (PRUint8)(decimal | 0x80);
		decimal >>= 7;
	    }
	    result_bytes += num_bytes;
	}
	++decimal_numbers;
	if (len > 0) { 
	    ++from;
	    --len;
	}
    } while (len > 0);
    
    if (to->data && to->len >= result_bytes) {
    	PORT_Memcpy(to->data, result, to->len = result_bytes);
	rv = SECSuccess;
    } else {
    	SECItem result_item = {siBuffer, NULL, 0 };
	result_item.data = result;
	result_item.len  = result_bytes;
	rv = SECITEM_CopyItem(pool, to, &result_item);
    }
    return rv;
}
