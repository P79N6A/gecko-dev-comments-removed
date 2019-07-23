









































typedef struct NSSLOWInitContextStr NSSLOWInitContext;
typedef struct NSSLOWHASHContextStr NSSLOWHASHContext;

NSSLOWInitContext *NSSLOW_Init(void);
void NSSLOW_Shutdown(NSSLOWInitContext *context);
void NSSLOW_Reset(NSSLOWInitContext *context);
NSSLOWHASHContext *NSSLOWHASH_NewContext(
			NSSLOWInitContext *initContext, 
			HASH_HashType hashType);
void NSSLOWHASH_Begin(NSSLOWHASHContext *context);
void NSSLOWHASH_Update(NSSLOWHASHContext *context, 
			const unsigned char *buf, 
			unsigned int len);
void NSSLOWHASH_End(NSSLOWHASHContext *context, 
			unsigned char *buf, 
			unsigned int *ret, unsigned int len);
void NSSLOWHASH_Destroy(NSSLOWHASHContext *context);
unsigned int NSSLOWHASH_Length(NSSLOWHASHContext *context); 
