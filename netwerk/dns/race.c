











































#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "nsIDNKitInterface.h"


#define RACE_2OCTET_MODE	0xd8
#define RACE_ESCAPE		0xff
#define RACE_ESCAPE_2ND		0x99




enum {
	compress_one,	
	compress_two,	
	compress_none	
};


idn_result_t
race_decode_decompress(const char *from, PRUint16 *buf, size_t buflen)
{
	PRUint16 *p = buf;
	unsigned int bitbuf = 0;
	int bitlen = 0;
	unsigned int i, j;
	size_t len;

	while (*from != '\0') {
		int c = *from++;
		int x;

		if ('a' <= c && c <= 'z')
			x = c - 'a';
		else if ('A' <= c && c <= 'Z')
			x = c - 'A';
		else if ('2' <= c && c <= '7')
			x = c - '2' + 26;
		else
			return (idn_invalid_encoding);

		bitbuf = (bitbuf << 5) + x;
		bitlen += 5;
		if (bitlen >= 8) {
			*p++ = (bitbuf >> (bitlen - 8)) & 0xff;
			bitlen -= 8;
		}
	}
	len = p - buf;

	



	


	if (buf[0] == RACE_2OCTET_MODE) {
		if ((len - 1) % 2 != 0)
			return (idn_invalid_encoding);
		for (i = 1, j = 0; i < len; i += 2, j++)
			buf[j] = (buf[i] << 8) + buf[i + 1];
		len = j;
	} else {
		PRUint16 c = buf[0] << 8;	

		for (i = 1, j = 0; i < len; j++) {
			if (buf[i] == RACE_ESCAPE) {
				if (i + 1 >= len)
					return (idn_invalid_encoding);
				else if (buf[i + 1] == RACE_ESCAPE_2ND)
					buf[j] = c | 0xff;
				else
					buf[j] = buf[i + 1];
				i += 2;

			} else if (buf[i] == 0x99 && c == 0x00) {
				


				return (idn_invalid_encoding);
				 
			} else {
				buf[j] = c | buf[i++];
			}
		}
		len = j;
	}
	buf[len] = '\0';

	return (idn_success);
}

idn_result_t
race_compress_encode(const PRUint16 *p, int compress_mode,
		     char *to, size_t tolen)
{
	PRUint32 bitbuf = *p++;	
	int bitlen = 8;			

	while (*p != '\0' || bitlen > 0) {
		unsigned int c = *p;

		if (c == '\0') {
			
			bitbuf <<= (5 - bitlen);
			bitlen = 5;
		} else if (compress_mode == compress_none) {
			
			bitbuf = (bitbuf << 16) | c;
			bitlen += 16;
			p++;
		} else {
			
			if (compress_mode == compress_two &&
			    (c & 0xff00) == 0) {
				
				bitbuf = (bitbuf << 16) | 0xff00 | c;
				bitlen += 16;
			} else if ((c & 0xff) == 0xff) {
				
				bitbuf = (bitbuf << 16) |
					(RACE_ESCAPE << 8) | RACE_ESCAPE_2ND;
				bitlen += 16;
			} else {
				
				bitbuf = (bitbuf << 8) | (c & 0xff);
				bitlen += 8;
			}
			p++;
		}

		


		while (bitlen >= 5) {
			int x;

			
			x = (bitbuf >> (bitlen - 5)) & 0x1f;
			bitlen -= 5;

			
			if (x < 26)
				x += 'a';
			else
				x = (x - 26) + '2';

			if (tolen < 1)
				return (idn_buffer_overflow);

			*to++ = x;
			tolen--;
		}
	}

	if (tolen <= 0)
		return (idn_buffer_overflow);

	*to = '\0';
	return (idn_success);
}

int
get_compress_mode(PRUint16 *p) {
	int zero = 0;
	unsigned int upper = 0;
	PRUint16 *modepos = p - 1;

	while (*p != '\0') {
		unsigned int hi = *p++ & 0xff00;

		if (hi == 0) {
			zero++;
		} else if (hi == upper) {
			;
		} else if (upper == 0) {
			upper = hi;
		} else {
			*modepos = RACE_2OCTET_MODE;
			return (compress_none);
		}
	}
	*modepos = upper >> 8;
	if (upper > 0 && zero > 0)
		return (compress_two);
	else
		return (compress_one);
}
