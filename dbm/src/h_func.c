

































#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)hash_func.c	8.2 (Berkeley) 2/21/94";
#endif 

#ifndef macintosh
#include <sys/types.h>
#endif
#include "mcom_db.h"
#include "hash.h"
#include "page.h"


#if 0
static uint32 hash1 __P((const void *, size_t));
static uint32 hash2 __P((const void *, size_t));
static uint32 hash3 __P((const void *, size_t));
#endif
static uint32 hash4 __P((const void *, size_t));


uint32 (*__default_hash) __P((const void *, size_t)) = hash4;










#define PRIME1		37
#define PRIME2		1048583

#if 0
static uint32
hash1(const void *keyarg, register size_t len)
{
	register const uint8 *key;
	register uint32 h;

	
	for (key = (const uint8 *)keyarg, h = 0; len--;)
		h = h * PRIME1 ^ (*key++ - ' ');
	h %= PRIME2;
	return (h);
}




#define dcharhash(h, c)	((h) = 0x63c63cd9*(h) + 0x9c39c33d + (c))

static uint32
hash2(const void *keyarg, size_t len)
{
	register const uint8 *e, *key;
	register uint32 h;
	register uint8 c;

	key = (const uint8 *)keyarg;
	e = key + len;
	for (h = 0; key != e;) {
		c = *key++;
		if (!c && key > e)
			break;
		dcharhash(h, c);
	}
	return (h);
}










static uint32
hash3(const void *keyarg, register size_t len)
{
	register const uint8 *key;
	register size_t loop;
	register uint32 h;

#define HASHC   h = *key++ + 65599 * h

	h = 0;
	key = (const uint8 *)keyarg;
	if (len > 0) {
		loop = (len + 8 - 1) >> 3;

		switch (len & (8 - 1)) {
		case 0:
			do {
				HASHC;
				
		case 7:
				HASHC;
				
		case 6:
				HASHC;
				
		case 5:
				HASHC;
				
		case 4:
				HASHC;
				
		case 3:
				HASHC;
				
		case 2:
				HASHC;
				
		case 1:
				HASHC;
			} while (--loop);
		}
	}
	return (h);
}
#endif 


static uint32
hash4(const void *keyarg, register size_t len)
{
	register const uint8 *key;
	register size_t loop;
	register uint32 h;

#define HASH4a   h = (h << 5) - h + *key++;
#define HASH4b   h = (h << 5) + h + *key++;
#define HASH4 HASH4b

	h = 0;
	key = (const uint8 *)keyarg;
	if (len > 0) {
		loop = (len + 8 - 1) >> 3;

		switch (len & (8 - 1)) {
		case 0:
			do {
				HASH4;
				
		case 7:
				HASH4;
				
		case 6:
				HASH4;
				
		case 5:
				HASH4;
				
		case 4:
				HASH4;
				
		case 3:
				HASH4;
				
		case 2:
				HASH4;
				
		case 1:
				HASH4;
			} while (--loop);
		}
	}
	return (h);
}
