









































#include "seccomon.h"
#include "secitem.h"
#include "base64.h"
#include "secerr.h"

SECItem *
SECITEM_AllocItem(PRArenaPool *arena, SECItem *item, unsigned int len)
{
    SECItem *result = NULL;
    void *mark = NULL;

    if (arena != NULL) {
	mark = PORT_ArenaMark(arena);
    }

    if (item == NULL) {
	if (arena != NULL) {
	    result = PORT_ArenaZAlloc(arena, sizeof(SECItem));
	} else {
	    result = PORT_ZAlloc(sizeof(SECItem));
	}
	if (result == NULL) {
	    goto loser;
	}
    } else {
	PORT_Assert(item->data == NULL);
	result = item;
    }

    result->len = len;
    if (len) {
	if (arena != NULL) {
	    result->data = PORT_ArenaAlloc(arena, len);
	} else {
	    result->data = PORT_Alloc(len);
	}
	if (result->data == NULL) {
	    goto loser;
	}
    } else {
	result->data = NULL;
    }

    if (mark) {
	PORT_ArenaUnmark(arena, mark);
    }
    return(result);

loser:
    if ( arena != NULL ) {
	if (mark) {
	    PORT_ArenaRelease(arena, mark);
	}
	if (item != NULL) {
	    item->data = NULL;
	    item->len = 0;
	}
    } else {
	if (result != NULL) {
	    SECITEM_FreeItem(result, (item == NULL) ? PR_TRUE : PR_FALSE);
	}
	



    }
    return(NULL);
}

SECStatus
SECITEM_ReallocItem(PRArenaPool *arena, SECItem *item, unsigned int oldlen,
		    unsigned int newlen)
{
    PORT_Assert(item != NULL);
    if (item == NULL) {
	
	return SECFailure;
    }

    


    if (oldlen == 0) {
	PORT_Assert(item->data == NULL || item->len == 0);
	if (newlen == 0) {
	    
	    return SECSuccess;
	}
	item->len = newlen;
	if (arena != NULL) {
	    item->data = PORT_ArenaAlloc(arena, newlen);
	} else {
	    item->data = PORT_Alloc(newlen);
	}
    } else {
	if (arena != NULL) {
	    item->data = PORT_ArenaGrow(arena, item->data, oldlen, newlen);
	} else {
	    item->data = PORT_Realloc(item->data, newlen);
	}
    }

    if (item->data == NULL) {
	return SECFailure;
    }

    return SECSuccess;
}

SECComparison
SECITEM_CompareItem(const SECItem *a, const SECItem *b)
{
    unsigned m;
    SECComparison rv;

    if (a == b)
    	return SECEqual;
    if (!a || !a->len || !a->data) 
        return (!b || !b->len || !b->data) ? SECEqual : SECLessThan;
    if (!b || !b->len || !b->data) 
    	return SECGreaterThan;

    m = ( ( a->len < b->len ) ? a->len : b->len );
    
    rv = (SECComparison) PORT_Memcmp(a->data, b->data, m);
    if (rv) {
	return rv;
    }
    if (a->len < b->len) {
	return SECLessThan;
    }
    if (a->len == b->len) {
	return SECEqual;
    }
    return SECGreaterThan;
}

PRBool
SECITEM_ItemsAreEqual(const SECItem *a, const SECItem *b)
{
    if (a->len != b->len)
        return PR_FALSE;
    if (!a->len)
    	return PR_TRUE;
    if (!a->data || !b->data) {
        
	return (PRBool)(a->data == b->data);
    }
    return (PRBool)!PORT_Memcmp(a->data, b->data, a->len);
}

SECItem *
SECITEM_DupItem(const SECItem *from)
{
    return SECITEM_ArenaDupItem(NULL, from);
}

SECItem *
SECITEM_ArenaDupItem(PRArenaPool *arena, const SECItem *from)
{
    SECItem *to;
    
    if ( from == NULL ) {
	return(NULL);
    }
    
    if ( arena != NULL ) {
	to = (SECItem *)PORT_ArenaAlloc(arena, sizeof(SECItem));
    } else {
	to = (SECItem *)PORT_Alloc(sizeof(SECItem));
    }
    if ( to == NULL ) {
	return(NULL);
    }

    if ( arena != NULL ) {
	to->data = (unsigned char *)PORT_ArenaAlloc(arena, from->len);
    } else {
	to->data = (unsigned char *)PORT_Alloc(from->len);
    }
    if ( to->data == NULL ) {
	PORT_Free(to);
	return(NULL);
    }

    to->len = from->len;
    to->type = from->type;
    if ( to->len ) {
	PORT_Memcpy(to->data, from->data, to->len);
    }
    
    return(to);
}

SECStatus
SECITEM_CopyItem(PRArenaPool *arena, SECItem *to, const SECItem *from)
{
    to->type = from->type;
    if (from->data && from->len) {
	if ( arena ) {
	    to->data = (unsigned char*) PORT_ArenaAlloc(arena, from->len);
	} else {
	    to->data = (unsigned char*) PORT_Alloc(from->len);
	}
	
	if (!to->data) {
	    return SECFailure;
	}
	PORT_Memcpy(to->data, from->data, from->len);
	to->len = from->len;
    } else {
	to->data = 0;
	to->len = 0;
    }
    return SECSuccess;
}

void
SECITEM_FreeItem(SECItem *zap, PRBool freeit)
{
    if (zap) {
	PORT_Free(zap->data);
	zap->data = 0;
	zap->len = 0;
	if (freeit) {
	    PORT_Free(zap);
	}
    }
}

void
SECITEM_ZfreeItem(SECItem *zap, PRBool freeit)
{
    if (zap) {
	PORT_ZFree(zap->data, zap->len);
	zap->data = 0;
	zap->len = 0;
	if (freeit) {
	    PORT_ZFree(zap, sizeof(SECItem));
	}
    }
}








PLHashNumber PR_CALLBACK
SECITEM_Hash ( const void *key)
{
    const SECItem *item = (const SECItem *)key;
    PLHashNumber rv = 0;

    PRUint8 *data = (PRUint8 *)item->data;
    PRUint32 i;
    PRUint8 *rvc = (PRUint8 *)&rv;

    for( i = 0; i < item->len; i++ ) {
        rvc[ i % sizeof(rv) ] ^= *data;
        data++;
    }

    return rv;
}







PRIntn PR_CALLBACK
SECITEM_HashCompare ( const void *k1, const void *k2)
{
    const SECItem *i1 = (const SECItem *)k1;
    const SECItem *i2 = (const SECItem *)k2;

    return SECITEM_ItemsAreEqual(i1,i2);
}
