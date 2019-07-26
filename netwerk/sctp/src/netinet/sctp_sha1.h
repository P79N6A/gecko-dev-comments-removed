































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#endif


#ifndef __NETINET_SCTP_SHA1_H__
#define __NETINET_SCTP_SHA1_H__

#include <sys/types.h>
#if defined(SCTP_USE_NSS_SHA1)
#if defined(__Userspace_os_Darwin)



#define __APPLE__
#endif
#include <pk11pub.h>
#if defined(__Userspace_os_Darwin)
#undef __APPLE__
#endif
#elif defined(SCTP_USE_OPENSSL_SHA1)
#include <openssl/sha.h>
#endif

struct sctp_sha1_context {
#if defined(SCTP_USE_NSS_SHA1)
	struct PK11Context *pk11_ctx;
#elif defined(SCTP_USE_OPENSSL_SHA1)
	SHA_CTX sha_ctx;
#else
	unsigned int A;
	unsigned int B;
	unsigned int C;
	unsigned int D;
	unsigned int E;
	unsigned int H0;
	unsigned int H1;
	unsigned int H2;
	unsigned int H3;
	unsigned int H4;
	unsigned int words[80];
	unsigned int TEMP;
	
	char sha_block[64];
	
	int how_many_in_block;
	unsigned int running_total;
#endif
};

#if (defined(__APPLE__) && defined(KERNEL))
#ifndef _KERNEL
#define _KERNEL
#endif
#endif

#if defined(_KERNEL) || defined(__Userspace__)

void sctp_sha1_init(struct sctp_sha1_context *);
void sctp_sha1_update(struct sctp_sha1_context *, const unsigned char *, unsigned int);
void sctp_sha1_final(unsigned char *, struct sctp_sha1_context *);

#endif
#endif
