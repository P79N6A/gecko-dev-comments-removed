































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#endif

#include <netinet/sctp_sha1.h>
#if !defined(__Userspace_os_Windows)
#include <sys/param.h>
#if !defined(__Windows__)
#include <arpa/inet.h>
#endif
#else
#include <winsock2.h>
#endif
#if !defined(__Userspace__)
#include <sys/systm.h>
#endif
#include <string.h>
void
SHA1_Init(struct sha1_context *ctx)
{
	
	ctx->A = 0;
	ctx->B = 0;
	ctx->C = 0;
	ctx->D = 0;
	ctx->E = 0;
	ctx->H0 = H0INIT;
	ctx->H1 = H1INIT;
	ctx->H2 = H2INIT;
	ctx->H3 = H3INIT;
	ctx->H4 = H4INIT;
	ctx->TEMP = 0;
	memset(ctx->words, 0, sizeof(ctx->words));
	ctx->how_many_in_block = 0;
	ctx->running_total = 0;
}

static void
sha1_process_a_block(struct sha1_context *ctx, unsigned int *block)
{
	int i;

	
	
	for (i = 0; i < 16; i++) {
		ctx->words[i] = ntohl(block[i]);
	}
	
	for (i = 16; i < 80; i++) {
		ctx->words[i] = CSHIFT(1, ((ctx->words[(i - 3)]) ^
		    (ctx->words[(i - 8)]) ^
		    (ctx->words[(i - 14)]) ^
		    (ctx->words[(i - 16)])));
	}
	
	ctx->A = ctx->H0;
	ctx->B = ctx->H1;
	ctx->C = ctx->H2;
	ctx->D = ctx->H3;
	ctx->E = ctx->H4;

	
	for (i = 0; i < 80; i++) {
		if (i < 20) {
			ctx->TEMP = ((CSHIFT(5, ctx->A)) +
			    (F1(ctx->B, ctx->C, ctx->D)) +
			    (ctx->E) +
			    ctx->words[i] +
			    K1);
		} else if (i < 40) {
			ctx->TEMP = ((CSHIFT(5, ctx->A)) +
			    (F2(ctx->B, ctx->C, ctx->D)) +
			    (ctx->E) +
			    (ctx->words[i]) +
			    K2);
		} else if (i < 60) {
			ctx->TEMP = ((CSHIFT(5, ctx->A)) +
			    (F3(ctx->B, ctx->C, ctx->D)) +
			    (ctx->E) +
			    (ctx->words[i]) +
			    K3);
		} else {
			ctx->TEMP = ((CSHIFT(5, ctx->A)) +
			    (F4(ctx->B, ctx->C, ctx->D)) +
			    (ctx->E) +
			    (ctx->words[i]) +
			    K4);
		}
		ctx->E = ctx->D;
		ctx->D = ctx->C;
		ctx->C = CSHIFT(30, ctx->B);
		ctx->B = ctx->A;
		ctx->A = ctx->TEMP;
	}
	
	ctx->H0 = (ctx->H0) + (ctx->A);
	ctx->H1 = (ctx->H1) + (ctx->B);
	ctx->H2 = (ctx->H2) + (ctx->C);
	ctx->H3 = (ctx->H3) + (ctx->D);
	ctx->H4 = (ctx->H4) + (ctx->E);
}


void
SHA1_Update(struct sha1_context *ctx, const unsigned char *ptr, int siz)
{
	int number_left, left_to_fill;

	number_left = siz;
	while (number_left > 0) {
		left_to_fill = sizeof(ctx->sha_block) - ctx->how_many_in_block;
		if (left_to_fill > number_left) {
			
			memcpy(&ctx->sha_block[ctx->how_many_in_block],
			    ptr, number_left);
			ctx->how_many_in_block += number_left;
			ctx->running_total += number_left;
			number_left = 0;
			break;
		} else {
			
			memcpy(&ctx->sha_block[ctx->how_many_in_block],
			    ptr, left_to_fill);
			sha1_process_a_block(ctx,
			    (unsigned int *)ctx->sha_block);
			number_left -= left_to_fill;
			ctx->running_total += left_to_fill;
			ctx->how_many_in_block = 0;
			ptr = (const unsigned char *)(ptr + left_to_fill);
		}
	}
}

void
SHA1_Final(unsigned char *digest, struct sha1_context *ctx)
{
	











	int left_to_fill;
	unsigned int i, *ptr;

	if (ctx->how_many_in_block > 55) {
		




		left_to_fill = sizeof(ctx->sha_block) - ctx->how_many_in_block;
		if (left_to_fill == 0) {
			
			sha1_process_a_block(ctx,
			    (unsigned int *)ctx->sha_block);
			
			ctx->sha_block[0] = '\x80';
			for (i = 1; i < sizeof(ctx->sha_block); i++) {
				ctx->sha_block[i] = 0x0;
			}
		} else if (left_to_fill == 1) {
			ctx->sha_block[ctx->how_many_in_block] = '\x80';
			sha1_process_a_block(ctx,
			    (unsigned int *)ctx->sha_block);
			
			memset(ctx->sha_block, 0, sizeof(ctx->sha_block));
		} else {
			ctx->sha_block[ctx->how_many_in_block] = '\x80';
			for (i = (ctx->how_many_in_block + 1);
			    i < sizeof(ctx->sha_block);
			    i++) {
				ctx->sha_block[i] = 0x0;
			}
			sha1_process_a_block(ctx,
			    (unsigned int *)ctx->sha_block);
			
			memset(ctx->sha_block, 0, sizeof(ctx->sha_block));
		}
		
		ctx->running_total *= 8;
		ptr = (unsigned int *)&ctx->sha_block[60];
		*ptr = htonl(ctx->running_total);
		sha1_process_a_block(ctx, (unsigned int *)ctx->sha_block);
	} else {
		





		ctx->sha_block[ctx->how_many_in_block] = '\x80';
		for (i = (ctx->how_many_in_block + 1);
		    i < sizeof(ctx->sha_block);
		    i++) {
			ctx->sha_block[i] = 0x0;
		}
		
		ctx->running_total *= 8;
		ptr = (unsigned int *)&ctx->sha_block[60];
		*ptr = htonl(ctx->running_total);
		sha1_process_a_block(ctx, (unsigned int *)ctx->sha_block);
	}
	
	digest[3] = (ctx->H0 & 0xff);
	digest[2] = ((ctx->H0 >> 8) & 0xff);
	digest[1] = ((ctx->H0 >> 16) & 0xff);
	digest[0] = ((ctx->H0 >> 24) & 0xff);

	digest[7] = (ctx->H1 & 0xff);
	digest[6] = ((ctx->H1 >> 8) & 0xff);
	digest[5] = ((ctx->H1 >> 16) & 0xff);
	digest[4] = ((ctx->H1 >> 24) & 0xff);

	digest[11] = (ctx->H2 & 0xff);
	digest[10] = ((ctx->H2 >> 8) & 0xff);
	digest[9] = ((ctx->H2 >> 16) & 0xff);
	digest[8] = ((ctx->H2 >> 24) & 0xff);

	digest[15] = (ctx->H3 & 0xff);
	digest[14] = ((ctx->H3 >> 8) & 0xff);
	digest[13] = ((ctx->H3 >> 16) & 0xff);
	digest[12] = ((ctx->H3 >> 24) & 0xff);

	digest[19] = (ctx->H4 & 0xff);
	digest[18] = ((ctx->H4 >> 8) & 0xff);
	digest[17] = ((ctx->H4 >> 16) & 0xff);
	digest[16] = ((ctx->H4 >> 24) & 0xff);
}
