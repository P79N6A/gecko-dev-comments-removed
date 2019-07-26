































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#endif

#include <netinet/sctp_os.h>

#include <netinet/sctp_constants.h>
#ifdef USE_MD5
#include <crypto/md5.h>
#else
#if !defined(__APPLE__) && !defined(__Userspace__)
#include <netinet/sctp_sha1.h>
#endif
#endif				
#include <netinet/sctp_hashdriver.h>











void
sctp_hash_digest(char *key, int key_len, char *text, int text_len,
    unsigned char *digest)
{
#ifdef USE_MD5
	md5_ctxt context;

#else
#if defined(__APPLE__) || defined(__Userspace__)
	SHA1_CTX context;
#else
	struct sha1_context context;
#endif

#endif				
	
	unsigned char k_ipad[65];

	
	unsigned char k_opad[65];
	unsigned char tk[20];
	int i;

	if (key_len > 64) {
#ifdef USE_MD5
		md5_ctxt tctx;

		MD5Init(&tctx);
		MD5Update(&tctx, key, key_len);
		MD5Final(tk, &tctx);
		key = tk;
		key_len = 16;
#else
#if defined(__APPLE__) || defined(__Userspace__)
		SHA1_CTX tctx;
#else
		struct sha1_context tctx;
#endif

		SHA1_Init(&tctx);
		SHA1_Update(&tctx, (unsigned char *)key, key_len);
		SHA1_Final(tk, &tctx);
		key = (char *)tk;
		key_len = 20;
#endif				
	}
	









	
	bzero(k_ipad, sizeof k_ipad);
	bzero(k_opad, sizeof k_opad);
	bcopy(key, k_ipad, key_len);
	bcopy(key, k_opad, key_len);

	
	for (i = 0; i < 64; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}
	


#ifdef USE_MD5
	MD5Init(&context);	
	MD5Update(&context, k_ipad, 64);	
	MD5Update(&context, text, text_len);	
	MD5Final(digest, &context);	
#else
	SHA1_Init(&context);	
	SHA1_Update(&context, k_ipad, 64);	
	SHA1_Update(&context,
	    (unsigned char *)text,
	    text_len);		
	SHA1_Final(digest, &context);	
#endif				

	


#ifdef USE_MD5
	MD5Init(&context);	
	MD5Update(&context, k_opad, 64);	
	MD5Update(&context, digest, 16);	
	MD5Final(digest, &context);	
#else
	SHA1_Init(&context);	
	SHA1_Update(&context, k_opad, 64);	
	SHA1_Update(&context,
	    (unsigned char *)digest, 20);	
	SHA1_Final(digest, &context);	
#endif				
}

void
sctp_hash_digest_m(char *key, int key_len, struct mbuf *m, int offset,
    unsigned char *digest)
{
	struct mbuf *m_at;

#ifdef USE_MD5
	md5_ctxt context;

#else
#if defined(__APPLE__) || defined(__Userspace__)
	SHA1_CTX context;
#else
	struct sha1_context context;
#endif

#endif				
	
	unsigned char k_ipad[65];

	
	unsigned char k_opad[65];
	unsigned char tk[20];
	int i;

	if (key_len > 64) {
#ifdef USE_MD5
		md5_ctxt tctx;

		MD5Init(&tctx);
		MD5Update(&tctx, key, key_len);
		MD5Final(tk, &tctx);
		key = tk;
		key_len = 16;
#else
#if defined(__APPLE__) || defined(__Userspace__)
		SHA1_CTX tctx;
#else
		struct sha1_context tctx;
#endif

		SHA1_Init(&tctx);
		SHA1_Update(&tctx, (unsigned char *)key, key_len);
		SHA1_Final(tk, &tctx);
		key = (char *)tk;
		key_len = 20;
#endif				
	}
	









	
	bzero(k_ipad, sizeof k_ipad);
	bzero(k_opad, sizeof k_opad);
	bcopy(key, k_ipad, key_len);
	bcopy(key, k_opad, key_len);

	
	for (i = 0; i < 64; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	
	m_at = m;
	while ((m_at != NULL) && (offset > SCTP_BUF_LEN(m_at))) {
		offset -= SCTP_BUF_LEN(m_at);	
		m_at = SCTP_BUF_NEXT(m_at);
	}
	


#ifdef USE_MD5
	MD5Init(&context);	
	MD5Update(&context, k_ipad, 64);	
	
	while (m_at != NULL) {
		
		MD5Update(&context, mtod(m_at, char *)+offset,
		    SCTP_BUF_LEN(m_at) - offset);
		
		offset = 0;
		m_at = SCTP_BUF_NEXT(m_at);
	}
	
	MD5Final(digest, &context);	
#else
	SHA1_Init(&context);	
	SHA1_Update(&context, k_ipad, 64);	
	
	while (m_at != NULL) {
		
		SHA1_Update(&context, mtod(m_at, unsigned char *)+offset,
		    SCTP_BUF_LEN(m_at) - offset);
		
		offset = 0;
		m_at = SCTP_BUF_NEXT(m_at);
	}
	
	SHA1_Final(digest, &context);	
#endif				

	


#ifdef USE_MD5
	MD5Init(&context);	
	MD5Update(&context, k_opad, 64);	
	MD5Update(&context, digest, 16);	
	MD5Final(digest, &context);	
#else
	SHA1_Init(&context);	
	SHA1_Update(&context, k_opad, 64);	
	SHA1_Update(&context, digest, 20);	
	SHA1_Final(digest, &context);	
#endif				
}
