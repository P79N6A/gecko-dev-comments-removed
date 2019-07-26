































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_auth.h 257804 2013-11-07 18:50:11Z tuexen $");
#endif

#ifndef _NETINET_SCTP_AUTH_H_
#define _NETINET_SCTP_AUTH_H_

#include <netinet/sctp_os.h>


#define SCTP_AUTH_DIGEST_LEN_SHA1	20
#define SCTP_AUTH_DIGEST_LEN_SHA256	32
#define SCTP_AUTH_DIGEST_LEN_MAX	SCTP_AUTH_DIGEST_LEN_SHA256


#define SCTP_AUTH_RANDOM_SIZE_DEFAULT	32
#define SCTP_AUTH_RANDOM_SIZE_REQUIRED	32


typedef union sctp_hash_context {
	SCTP_SHA1_CTX sha1;
#if defined(SCTP_SUPPORT_HMAC_SHA256)
	SCTP_SHA256_CTX sha256;
#endif
} sctp_hash_context_t;

typedef struct sctp_key {
	uint32_t keylen;
	uint8_t key[];
} sctp_key_t;

typedef struct sctp_shared_key {
	LIST_ENTRY(sctp_shared_key) next;
	sctp_key_t *key;	
	uint32_t refcount;	
	uint16_t keyid;		
	uint8_t deactivated;	
} sctp_sharedkey_t;

LIST_HEAD(sctp_keyhead, sctp_shared_key);


typedef struct sctp_auth_chklist {
	uint8_t chunks[256];
	uint8_t num_chunks;
} sctp_auth_chklist_t;


typedef struct sctp_hmaclist {
	uint16_t max_algo;	
	uint16_t num_algo;	
	uint16_t hmac[];
} sctp_hmaclist_t;


typedef struct sctp_authinformation {
	sctp_key_t *random;	
	uint32_t random_len;	
	sctp_key_t *peer_random;
	sctp_key_t *assoc_key;	
	sctp_key_t *recv_key;	
	uint16_t active_keyid;	
	uint16_t assoc_keyid;	
	uint16_t recv_keyid;	
} sctp_authinfo_t;






#define sctp_auth_is_required_chunk(chunk, list) ((list == NULL) ? (0) : (list->chunks[chunk] != 0))






extern sctp_auth_chklist_t *sctp_alloc_chunklist(void);
extern void sctp_free_chunklist(sctp_auth_chklist_t *chklist);
extern void sctp_clear_chunklist(sctp_auth_chklist_t *chklist);
extern sctp_auth_chklist_t *sctp_copy_chunklist(sctp_auth_chklist_t *chklist);
extern int sctp_auth_add_chunk(uint8_t chunk, sctp_auth_chklist_t *list);
extern int sctp_auth_delete_chunk(uint8_t chunk, sctp_auth_chklist_t *list);
extern size_t sctp_auth_get_chklist_size(const sctp_auth_chklist_t *list);
extern void sctp_auth_set_default_chunks(sctp_auth_chklist_t *list);
extern int sctp_serialize_auth_chunks(const sctp_auth_chklist_t *list,
    uint8_t *ptr);
extern int sctp_pack_auth_chunks(const sctp_auth_chklist_t *list,
    uint8_t *ptr);
extern int sctp_unpack_auth_chunks(const uint8_t *ptr, uint8_t num_chunks,
    sctp_auth_chklist_t *list);


extern sctp_key_t *sctp_alloc_key(uint32_t keylen);
extern void sctp_free_key(sctp_key_t *key);
extern void sctp_print_key(sctp_key_t *key, const char *str);
extern void sctp_show_key(sctp_key_t *key, const char *str);
extern sctp_key_t *sctp_generate_random_key(uint32_t keylen);
extern sctp_key_t *sctp_set_key(uint8_t *key, uint32_t keylen);
extern sctp_key_t *sctp_compute_hashkey(sctp_key_t *key1, sctp_key_t *key2,
    sctp_key_t *shared);


extern sctp_sharedkey_t *sctp_alloc_sharedkey(void);
extern void sctp_free_sharedkey(sctp_sharedkey_t *skey);
extern sctp_sharedkey_t *sctp_find_sharedkey(struct sctp_keyhead *shared_keys,
    uint16_t key_id);
extern int sctp_insert_sharedkey(struct sctp_keyhead *shared_keys,
    sctp_sharedkey_t *new_skey);
extern int sctp_copy_skeylist(const struct sctp_keyhead *src,
    struct sctp_keyhead *dest);

extern void sctp_auth_key_acquire(struct sctp_tcb *stcb, uint16_t keyid);
extern void sctp_auth_key_release(struct sctp_tcb *stcb, uint16_t keyid,
    int so_locked);



extern sctp_hmaclist_t *sctp_alloc_hmaclist(uint8_t num_hmacs);
extern void sctp_free_hmaclist(sctp_hmaclist_t *list);
extern int sctp_auth_add_hmacid(sctp_hmaclist_t *list, uint16_t hmac_id);
extern sctp_hmaclist_t *sctp_copy_hmaclist(sctp_hmaclist_t *list);
extern sctp_hmaclist_t *sctp_default_supported_hmaclist(void);
extern uint16_t sctp_negotiate_hmacid(sctp_hmaclist_t *peer,
    sctp_hmaclist_t *local);
extern int sctp_serialize_hmaclist(sctp_hmaclist_t *list, uint8_t *ptr);
extern int sctp_verify_hmac_param(struct sctp_auth_hmac_algo *hmacs,
    uint32_t num_hmacs);

extern sctp_authinfo_t *sctp_alloc_authinfo(void);
extern void sctp_free_authinfo(sctp_authinfo_t *authinfo);


extern uint32_t sctp_get_auth_chunk_len(uint16_t hmac_algo);
extern uint32_t sctp_get_hmac_digest_len(uint16_t hmac_algo);
extern uint32_t sctp_hmac(uint16_t hmac_algo, uint8_t *key, uint32_t keylen,
    uint8_t *text, uint32_t textlen, uint8_t *digest);
extern int sctp_verify_hmac(uint16_t hmac_algo, uint8_t *key, uint32_t keylen,
    uint8_t *text, uint32_t textlen, uint8_t *digest, uint32_t digestlen);
extern uint32_t sctp_compute_hmac(uint16_t hmac_algo, sctp_key_t *key,
    uint8_t *text, uint32_t textlen, uint8_t *digest);
extern int sctp_auth_is_supported_hmac(sctp_hmaclist_t *list, uint16_t id);


extern uint32_t sctp_hmac_m(uint16_t hmac_algo, uint8_t *key, uint32_t keylen,
    struct mbuf *m, uint32_t m_offset, uint8_t *digest, uint32_t trailer);
extern uint32_t sctp_compute_hmac_m(uint16_t hmac_algo, sctp_key_t *key,
    struct mbuf *m, uint32_t m_offset, uint8_t *digest);




extern void sctp_clear_cachedkeys(struct sctp_tcb *stcb, uint16_t keyid);
extern void sctp_clear_cachedkeys_ep(struct sctp_inpcb *inp, uint16_t keyid);
extern int sctp_delete_sharedkey(struct sctp_tcb *stcb, uint16_t keyid);
extern int sctp_delete_sharedkey_ep(struct sctp_inpcb *inp, uint16_t keyid);
extern int sctp_auth_setactivekey(struct sctp_tcb *stcb, uint16_t keyid);
extern int sctp_auth_setactivekey_ep(struct sctp_inpcb *inp, uint16_t keyid);
extern int sctp_deact_sharedkey(struct sctp_tcb *stcb, uint16_t keyid);
extern int sctp_deact_sharedkey_ep(struct sctp_inpcb *inp, uint16_t keyid);

extern void sctp_auth_get_cookie_params(struct sctp_tcb *stcb, struct mbuf *m,
    uint32_t offset, uint32_t length);
extern void sctp_fill_hmac_digest_m(struct mbuf *m, uint32_t auth_offset,
    struct sctp_auth_chunk *auth, struct sctp_tcb *stcb, uint16_t key_id);
extern struct mbuf *sctp_add_auth_chunk(struct mbuf *m, struct mbuf **m_end,
    struct sctp_auth_chunk **auth_ret, uint32_t *offset,
    struct sctp_tcb *stcb, uint8_t chunk);
extern int sctp_handle_auth(struct sctp_tcb *stcb, struct sctp_auth_chunk *ch,
    struct mbuf *m, uint32_t offset);
extern void sctp_notify_authentication(struct sctp_tcb *stcb,
    uint32_t indication, uint16_t keyid, uint16_t alt_keyid, int so_locked);
extern int sctp_validate_init_auth_params(struct mbuf *m, int offset,
    int limit);
extern void sctp_initialize_auth_params(struct sctp_inpcb *inp,
    struct sctp_tcb *stcb);


#ifdef SCTP_HMAC_TEST
extern void sctp_test_hmac_sha1(void);
extern void sctp_test_authkey(void);
#endif
#endif 
