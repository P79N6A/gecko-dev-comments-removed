









































#include "nssrenam.h"
#include "jar.h"
#include "jarint.h"
#include "certdb.h"
#include "certt.h"
#include "secpkcs7.h"


#include "secder.h"


#define CERTDB_USER (1<<6)

#define SZ 512

static int
jar_validate_pkcs7(JAR *jar, JAR_Signer *signer, char *data, long length);

static void
jar_catch_bytes(void *arg, const char *buf, unsigned long len);

static int
jar_gather_signers(JAR *jar, JAR_Signer *signer, SEC_PKCS7ContentInfo *cinfo);

static char *
jar_eat_line(int lines, int eating, char *data, long *len);

static JAR_Digest *
jar_digest_section(char *manifest, long length);

static JAR_Digest *jar_get_mf_digest(JAR *jar, char *path);

static int
jar_parse_digital_signature(char *raw_manifest, JAR_Signer *signer,
			    long length, JAR *jar);

static int
jar_add_cert(JAR *jar, JAR_Signer *signer, int type, CERTCertificate *cert);

static CERTCertificate *
jar_get_certificate(JAR *jar, long keylen, void *key, int *result);

static char * jar_cert_element(char *name, char *tag, int occ);

static char *jar_choose_nickname(CERTCertificate *cert);

static char *jar_basename(const char *path);

static int
jar_signal(int status, JAR *jar, const char *metafile, char *pathname);

#ifdef DEBUG
static int jar_insanity_check(char *data, long length);
#endif

int
jar_parse_mf(JAR *jar, char *raw_manifest, long length,
	     const char *path, const char *url);

int
jar_parse_sf(JAR *jar, char *raw_manifest, long length,
	     const char *path, const char *url);

int
jar_parse_sig(JAR *jar, const char *path, char *raw_manifest,
	      long length);

int
jar_parse_any(JAR *jar, int type, JAR_Signer *signer,
	      char *raw_manifest, long length, const char *path,
	  const char *url);

static int
jar_internal_digest(JAR *jar, const char *path, char *x_name, JAR_Digest *dig);











int
JAR_parse_manifest(JAR *jar, char *raw_manifest, long length,
		   const char *path, const char *url)
{
    int filename_free = 0;

    


    if (jar->filename == NULL && path) {
	jar->filename = PORT_Strdup(path);
	if (jar->filename == NULL)
	    return JAR_ERR_MEMORY;
	filename_free = 1;
    }

    


    if (jar->url == NULL && url) {
	jar->url = PORT_Strdup(url);
	if (jar->url == NULL) {
	    if (filename_free) {
		PORT_Free(jar->filename);
	    }
	    return JAR_ERR_MEMORY;
	}
    }

    


    if (!PORT_Strncasecmp (raw_manifest, "Manifest-Version:", 17)) {
	return jar_parse_mf(jar, raw_manifest, length, path, url);
    }
    else if (!PORT_Strncasecmp (raw_manifest, "Signature-Version:", 18))
    {
	return jar_parse_sf(jar, raw_manifest, length, path, url);
    } else {
	
	return jar_parse_sig(jar, path, raw_manifest, length);
    }
}








int
jar_parse_sig(JAR *jar, const char *path, char *raw_manifest,
	      long length)
{
    JAR_Signer *signer;
    int status = JAR_ERR_ORDER;

    if (length <= 128) {
	
	return JAR_ERR_SIG;
    }

    

    if (jar->globalmeta == NULL)
	return JAR_ERR_ORDER;

    


    if (path) {
	char *owner;
	owner = jar_basename(path);

	if (owner == NULL)
	    return JAR_ERR_MEMORY;

	signer = jar_get_signer(jar, owner);
	PORT_Free(owner);
    } else
	signer = jar_get_signer(jar, "*");

    if (signer == NULL)
	return JAR_ERR_ORDER;


    



    if (length > 64000) {
	
	return JAR_ERR_SIG;
    }

    
    status = jar_parse_digital_signature(raw_manifest, signer, length, jar);

    return status;
}








int
jar_parse_mf(JAR *jar, char *raw_manifest, long length,
	     const char *path, const char *url)
{
    if (jar->globalmeta) {
	
	return JAR_ERR_ORDER;
    }

    
    jar->globalmeta = jar_digest_section(raw_manifest, length);
    if (jar->globalmeta == NULL)
	return JAR_ERR_MEMORY;
    return jar_parse_any(jar, jarTypeMF, NULL, raw_manifest, length,
			 path, url);
}








int
jar_parse_sf(JAR *jar, char *raw_manifest, long length,
	     const char *path, const char *url)
{
    JAR_Signer *signer = NULL;
    int status = JAR_ERR_MEMORY;

    if (jar->globalmeta == NULL) {
	
	return JAR_ERR_ORDER;
    }

    signer = JAR_new_signer();
    if (signer == NULL)
	goto loser;

    if (path) {
	signer->owner = jar_basename(path);
	if (signer->owner == NULL)
	    goto loser;
    }

    


    if (jar_get_signer(jar, signer->owner)) {
	
	status = JAR_ERR_ORDER;
	goto loser;
    }

    
    signer->digest = JAR_calculate_digest (raw_manifest, length);
    if (signer->digest == NULL)
	goto loser;

    
    ADDITEM(jar->signers, jarTypeOwner, signer->owner, signer,
	    sizeof (JAR_Signer));

    return jar_parse_any(jar, jarTypeSF, signer, raw_manifest, length,
			 path, url);

loser:
    if (signer)
	JAR_destroy_signer (signer);
    return status;
}







int 
jar_parse_any(JAR *jar, int type, JAR_Signer *signer, 
              char *raw_manifest, long length, const char *path, 
	      const char *url)
{
    int status;
    long raw_len;
    JAR_Digest *dig, *mfdig = NULL;
    char line [SZ];
    char x_name [SZ], x_md5 [SZ], x_sha [SZ];
    char *x_info;
    char *sf_md5 = NULL, *sf_sha1 = NULL;

    *x_name = 0;
    *x_md5 = 0;
    *x_sha = 0;

    PORT_Assert( length > 0 );
    raw_len = length;

#ifdef DEBUG
    if ((status = jar_insanity_check(raw_manifest, raw_len)) < 0)
	return status;
#endif

    
    raw_manifest = jar_eat_line(0, PR_TRUE, raw_manifest, &raw_len);

    
    
    while (raw_len > 0) {
	JAR_Metainfo *met;

	raw_manifest = jar_eat_line(1, PR_TRUE, raw_manifest, &raw_len);
	if (raw_len <= 0 || !*raw_manifest)
	    break;

	met = PORT_ZNew(JAR_Metainfo);
	if (met == NULL)
	    return JAR_ERR_MEMORY;

	
	if (PORT_Strlen (raw_manifest) >= SZ) {
	    
	    PORT_Free(met);
	    continue;
	}

	PORT_Strcpy (line, raw_manifest);
	x_info = line;

	while (*x_info && *x_info != ' ' && *x_info != '\t' && *x_info != ':')
	    x_info++;

	if (*x_info)
	    *x_info++ = 0;

	while (*x_info == ' ' || *x_info == '\t')
	    x_info++;

	
	met->header = PORT_Strdup(line);
	met->info = PORT_Strdup(x_info);

	if (type == jarTypeMF) {
	    ADDITEM (jar->metainfo, jarTypeMeta,
	     NULL, met, sizeof (JAR_Metainfo));
	}

	


	if (type == jarTypeSF) {
	    if (!PORT_Strcasecmp(line, "MD5-Digest"))
		sf_md5 = (char *) met->info;

	    if (!PORT_Strcasecmp(line, "SHA1-Digest") || !PORT_Strcasecmp (line, "SHA-Digest"))
		sf_sha1 = (char *) met->info;
	}

	if (type != jarTypeMF) {
	    PORT_Free(met->header);
	    if (type != jarTypeSF) {
		PORT_Free(met->info);
	    }
	    PORT_Free(met);
	}
    }

    if (type == jarTypeSF && jar->globalmeta) {
	


	int match = 0;
	JAR_Digest *glob = jar->globalmeta;

	if (sf_md5) {
	    unsigned int md5_length;
	    unsigned char *md5_digest;

	    md5_digest = ATOB_AsciiToData (sf_md5, &md5_length);
	    PORT_Assert( md5_length == MD5_LENGTH );

	    if (md5_length != MD5_LENGTH)
		return JAR_ERR_CORRUPT;

	    match = PORT_Memcmp(md5_digest, glob->md5, MD5_LENGTH);
	}

	if (sf_sha1 && match == 0) {
	    unsigned int sha1_length;
	    unsigned char *sha1_digest;

	    sha1_digest = ATOB_AsciiToData (sf_sha1, &sha1_length);
	    PORT_Assert( sha1_length == SHA1_LENGTH );

	    if (sha1_length != SHA1_LENGTH)
		return JAR_ERR_CORRUPT;

	    match = PORT_Memcmp(sha1_digest, glob->sha1, SHA1_LENGTH);
	}

	if (match != 0) {
	    
	    jar->valid = JAR_ERR_METADATA;
	    return JAR_ERR_METADATA;
	}
    }

    
    while (raw_len > 0) {
	*x_md5 = 0;
	*x_sha = 0;
	*x_name = 0;

	


	if (type == jarTypeMF) {
	    char *sec;
	    long sec_len = raw_len;

	    if (!*raw_manifest || *raw_manifest == '\n') {
		
		sec = jar_eat_line(1, PR_FALSE, raw_manifest, &sec_len);
	    } else
		sec = raw_manifest;

	    if (sec_len > 0 && !PORT_Strncasecmp(sec, "Name:", 5)) {
		if (type == jarTypeMF)
		    mfdig = jar_digest_section(sec, sec_len);
		else
		    mfdig = NULL;
	    }
	}


	while (raw_len > 0) {
	    raw_manifest = jar_eat_line(1, PR_TRUE, raw_manifest, &raw_len);
	    if (raw_len <= 0 || !*raw_manifest)
		break; 

	    if (PORT_Strlen(raw_manifest) >= SZ) {
		
		continue;
	    }

	    
	    PORT_Strcpy(line, raw_manifest);
	    x_info = line;

	    while (*x_info && *x_info != ' ' && *x_info != '\t' && 
	           *x_info != ':')
		x_info++;

	    if (*x_info)
		*x_info++ = 0;

	    while (*x_info == ' ' || *x_info == '\t')
		x_info++;

	    if (!PORT_Strcasecmp(line, "Name"))
		PORT_Strcpy(x_name, x_info);
	    else if (!PORT_Strcasecmp(line, "MD5-Digest"))
		PORT_Strcpy(x_md5, x_info);
	    else if (!PORT_Strcasecmp(line, "SHA1-Digest")
		  || !PORT_Strcasecmp(line, "SHA-Digest"))
		PORT_Strcpy(x_sha, x_info);

	    

	    else if (!PORT_Strcasecmp(line, "Digest-Algorithms")
		  || !PORT_Strcasecmp(line, "Hash-Algorithms"))
		continue;

	    

	    else if (type == jarTypeMF) {
		JAR_Metainfo *met;

		
		met = PORT_ZNew(JAR_Metainfo);
		if (met == NULL)
		    return JAR_ERR_MEMORY;

		
		if ((met->header = PORT_Strdup(line)) == NULL) {
		    PORT_Free(met);
		    return JAR_ERR_MEMORY;
		}

		if ((met->info = PORT_Strdup(x_info)) == NULL) {
		    PORT_Free(met->header);
		    PORT_Free(met);
		    return JAR_ERR_MEMORY;
		}

		ADDITEM (jar->metainfo, jarTypeMeta,
		x_name, met, sizeof (JAR_Metainfo));
	    }
	}

	if (!x_name || !*x_name) {
	    

	    continue;
	}

	dig = PORT_ZNew(JAR_Digest);
	if (dig == NULL)
	    return JAR_ERR_MEMORY;

	if (*x_md5) {
	    unsigned int binary_length;
	    unsigned char *binary_digest;

	    binary_digest = ATOB_AsciiToData (x_md5, &binary_length);
	    PORT_Assert( binary_length == MD5_LENGTH );
	    if (binary_length != MD5_LENGTH) {
		PORT_Free(dig);
		return JAR_ERR_CORRUPT;
	    }
	    memcpy (dig->md5, binary_digest, MD5_LENGTH);
	    dig->md5_status = jarHashPresent;
	}

	if (*x_sha ) {
	    unsigned int binary_length;
	    unsigned char *binary_digest;

	    binary_digest = ATOB_AsciiToData (x_sha, &binary_length);
	    PORT_Assert( binary_length == SHA1_LENGTH );
	    if (binary_length != SHA1_LENGTH) {
		PORT_Free(dig);
		return JAR_ERR_CORRUPT;
	    }
	    memcpy (dig->sha1, binary_digest, SHA1_LENGTH);
	    dig->sha1_status = jarHashPresent;
	}

	PORT_Assert( type == jarTypeMF || type == jarTypeSF );
	if (type == jarTypeMF) {
	    ADDITEM (jar->hashes, jarTypeMF, x_name, dig, sizeof (JAR_Digest));
	} else if (type == jarTypeSF) {
	    ADDITEM (signer->sf, jarTypeSF, x_name, dig, sizeof (JAR_Digest));
	} else {
	    PORT_Free(dig);
	    return JAR_ERR_ORDER;
	}

	

	if (type == jarTypeMF && mfdig) {
	    ADDITEM (jar->manifest, jarTypeSect,
	    x_name, mfdig, sizeof (JAR_Digest));
	    mfdig = NULL;
	}

	



	if (type == jarTypeSF) {
	    if ((status = jar_internal_digest(jar, path, x_name, dig)) < 0)
		return status;
	}
    }

    return 0;
}

static int
jar_internal_digest(JAR *jar, const char *path, char *x_name, JAR_Digest *dig)
{
    int cv;
    int status;

    JAR_Digest *savdig;

    savdig = jar_get_mf_digest(jar, x_name);
    if (savdig == NULL) {
	
	status = jar_signal(JAR_ERR_ENTRY, jar, path, x_name);
	if (status < 0)
	    return 0; 
	return status;
    }

    
    if (dig->md5_status) {
	cv = PORT_Memcmp(savdig->md5, dig->md5, MD5_LENGTH);
	
	if (cv) {
	    status = jar_signal(JAR_ERR_HASH, jar, path, x_name);

	    
	    dig->md5_status = jarHashBad;
	    savdig->md5_status = jarHashBad;

	    if (status < 0)
		return 0; 
	    return status;
	}
    }

    
    if (dig->sha1_status) {
	cv = PORT_Memcmp(savdig->sha1, dig->sha1, SHA1_LENGTH);
	
	if (cv) {
	    status = jar_signal(JAR_ERR_HASH, jar, path, x_name);

	    
	    dig->sha1_status = jarHashBad;
	    savdig->sha1_status = jarHashBad;

	    if (status < 0)
		return 0; 
	    return status;
	}
    }
    return 0;
}

#ifdef DEBUG









static int
jar_insanity_check(char *data, long length)
{
    int c;
    long off;

    for (off = 0; off < length; off++) {
	c = data [off];
	if (c == '\n' || c == '\r' || (c >= ' ' && c <= 128))
	    continue;
	return JAR_ERR_CORRUPT;
    }
    return 0;
}
#endif








static int
jar_parse_digital_signature(char *raw_manifest, JAR_Signer *signer,
			    long length, JAR *jar)
{
    return jar_validate_pkcs7 (jar, signer, raw_manifest, length);
}










static int
jar_add_cert(JAR *jar, JAR_Signer *signer, int type, CERTCertificate *cert)
{
    JAR_Cert *fing;
    unsigned char *keyData;

    if (cert == NULL)
	return JAR_ERR_ORDER;

    fing = PORT_ZNew(JAR_Cert);
    if (fing == NULL)
	goto loser;

    fing->cert = CERT_DupCertificate (cert);

    
    fing->length = cert->derIssuer.len + 2 + cert->serialNumber.len;
    fing->key = keyData = (unsigned char *) PORT_ZAlloc(fing->length);
    if (fing->key == NULL)
	goto loser;
    keyData[0] = ((cert->derIssuer.len) >> 8) & 0xff;
    keyData[1] = ((cert->derIssuer.len) & 0xff);
    PORT_Memcpy(&keyData[2], cert->derIssuer.data, cert->derIssuer.len);
    PORT_Memcpy(&keyData[2+cert->derIssuer.len], cert->serialNumber.data,
		cert->serialNumber.len);

    ADDITEM (signer->certs, type, NULL, fing, sizeof (JAR_Cert));
    return 0;

loser:
    if (fing) {
	if (fing->cert)
	    CERT_DestroyCertificate (fing->cert);
	PORT_Free(fing);
    }
    return JAR_ERR_MEMORY;
}






















static char *
jar_eat_line(int lines, int eating, char *data, long *len)
{
    char *start = data;
    long maxLen = *len;

    if (maxLen <= 0)
	return start;

#define GO_ON ((data - start) < maxLen)

    

    for ( ; lines > 0; lines--) {
	while (GO_ON && *data && *data != '\r' && *data != '\n')
	    data++;

	
	if (GO_ON && *data == '\r')
	    data++;

	
	if (GO_ON && *data == '\n')
	    data++;

	
	while (GO_ON && !*data)
	    data++;
    }
    maxLen -= data - start;           
    *len  = maxLen;
    start = data;                     
    if (maxLen > 0 && eating) {
	
	while (GO_ON && *data && *data != '\n' && *data != '\r')
	    data++;

	
	if (GO_ON && *data == '\r')
	    *data++ = 0;

	
	if (GO_ON && *data == '\n')
	    *data++ = 0;
    }
    return start;
}
#undef GO_ON








static JAR_Digest *
jar_digest_section(char *manifest, long length)
{
    long global_len;
    char *global_end;

    global_end = manifest;
    global_len = length;

    while (global_len > 0) {
	global_end = jar_eat_line(1, PR_FALSE, global_end, &global_len);
	if (global_len > 0 && (*global_end == 0 || *global_end == '\n'))
	    break;
    }
    return JAR_calculate_digest (manifest, global_end - manifest);
}








int PR_CALLBACK
JAR_verify_digest(JAR *jar, const char *name, JAR_Digest *dig)
{
    JAR_Item *it;
    JAR_Digest *shindig;
    ZZLink *link;
    ZZList *list = jar->hashes;
    int result1 = 0;
    int result2 = 0;


    if (jar->valid < 0) {
	
	return JAR_ERR_SIG;
    }
    if (ZZ_ListEmpty (list)) {
	
	return JAR_ERR_PNF;
    }

    for (link = ZZ_ListHead (list);
	     !ZZ_ListIterDone (list, link);
	     link = link->next) {
	it = link->thing;
	if (it->type == jarTypeMF
	    && it->pathname && !PORT_Strcmp(it->pathname, name)) {
	    shindig = (JAR_Digest *) it->data;
	    if (shindig->md5_status) {
		if (shindig->md5_status == jarHashBad)
		    return JAR_ERR_HASH;
		result1 = memcmp (dig->md5, shindig->md5, MD5_LENGTH);
	    }
	    if (shindig->sha1_status) {
		if (shindig->sha1_status == jarHashBad)
		    return JAR_ERR_HASH;
		result2 = memcmp (dig->sha1, shindig->sha1, SHA1_LENGTH);
	    }
	    return (result1 == 0 && result2 == 0) ? 0 : JAR_ERR_HASH;
	}
    }
    return JAR_ERR_PNF;
}








int PR_CALLBACK
JAR_cert_attribute(JAR *jar, jarCert attrib, long keylen, void *key,
		   void **result, unsigned long *length)
{
    int status = 0;
    char *ret = NULL;
    CERTCertificate *cert;
    CERTCertDBHandle *certdb;
    JAR_Digest *dig;
    SECItem hexme;

    *length = 0;

    if (attrib == 0 || key == 0)
	return JAR_ERR_GENERAL;

    if (attrib == jarCertJavaHack) {
	cert = (CERTCertificate *) NULL;
	certdb = JAR_open_database();

	if (certdb) {
	    cert = CERT_FindCertByNickname (certdb, key);
	    if (cert) {
		*length = cert->certKey.len;
		*result = (void *) PORT_ZAlloc(*length);
		if (*result)
		    PORT_Memcpy(*result, cert->certKey.data, *length);
		else {
		    JAR_close_database (certdb);
		    return JAR_ERR_MEMORY;
		}
	    }
	    JAR_close_database (certdb);
	}
	return cert ? 0 : JAR_ERR_GENERAL;
    }

    if (jar && jar->pkcs7 == 0)
	return JAR_ERR_GENERAL;

    cert = jar_get_certificate(jar, keylen, key, &status);
    if (cert == NULL || status < 0)
	return JAR_ERR_GENERAL;

#define SEP " <br> "
#define SEPLEN (PORT_Strlen(SEP))

    switch (attrib) {
    case jarCertCompany:
	ret = cert->subjectName;

	

	if (ret) {
	    int retlen = 0;

	    char *cer_ou1, *cer_ou2, *cer_ou3;
	    char *cer_cn, *cer_e, *cer_o, *cer_l;

	    cer_cn  = CERT_GetCommonName (&cert->subject);
	    cer_e   = CERT_GetCertEmailAddress (&cert->subject);
	    cer_ou3 = jar_cert_element(ret, "OU=", 3);
	    cer_ou2 = jar_cert_element(ret, "OU=", 2);
	    cer_ou1 = jar_cert_element(ret, "OU=", 1);
	    cer_o   = CERT_GetOrgName (&cert->subject);
	    cer_l   = CERT_GetCountryName (&cert->subject);

	    if (cer_cn)
		retlen += SEPLEN + PORT_Strlen(cer_cn);
	    if (cer_e)
		retlen += SEPLEN + PORT_Strlen(cer_e);
	    if (cer_ou1)
		retlen += SEPLEN + PORT_Strlen(cer_ou1);
	    if (cer_ou2)
		retlen += SEPLEN + PORT_Strlen(cer_ou2);
	    if (cer_ou3)
		retlen += SEPLEN + PORT_Strlen(cer_ou3);
	    if (cer_o)
		retlen += SEPLEN + PORT_Strlen(cer_o);
	    if (cer_l)
		retlen += SEPLEN + PORT_Strlen(cer_l);

	    ret = (char *) PORT_ZAlloc(1 + retlen);

	    if (cer_cn)  {
		PORT_Strcpy(ret, cer_cn);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_e)	 {
		PORT_Strcat(ret, cer_e);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_ou1) {
		PORT_Strcat(ret, cer_ou1);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_ou2) {
		PORT_Strcat(ret, cer_ou2);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_ou3) {
		PORT_Strcat(ret, cer_ou3);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_o)	 {
		PORT_Strcat(ret, cer_o);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_l)
		PORT_Strcat(ret, cer_l);

	    
	    *result = ret;
	    *length = PORT_Strlen(ret);
	    CERT_DestroyCertificate(cert);
	    return 0;
	}
	break;

    case jarCertCA:
	ret = cert->issuerName;
	if (ret) {
	    int retlen = 0;

	    char *cer_ou1, *cer_ou2, *cer_ou3;
	    char *cer_cn, *cer_e, *cer_o, *cer_l;

	    


	    cer_cn  = CERT_GetCommonName (&cert->issuer);
	    cer_e   = CERT_GetCertEmailAddress (&cert->issuer);
	    cer_ou3 = jar_cert_element(ret, "OU=", 3);
	    cer_ou2 = jar_cert_element(ret, "OU=", 2);
	    cer_ou1 = jar_cert_element(ret, "OU=", 1);
	    cer_o   = CERT_GetOrgName (&cert->issuer);
	    cer_l   = CERT_GetCountryName (&cert->issuer);

	    if (cer_cn)
		retlen += SEPLEN + PORT_Strlen(cer_cn);
	    if (cer_e)
		retlen += SEPLEN + PORT_Strlen(cer_e);
	    if (cer_ou1)
		retlen += SEPLEN + PORT_Strlen(cer_ou1);
	    if (cer_ou2)
		retlen += SEPLEN + PORT_Strlen(cer_ou2);
	    if (cer_ou3)
		retlen += SEPLEN + PORT_Strlen(cer_ou3);
	    if (cer_o)
		retlen += SEPLEN + PORT_Strlen(cer_o);
	    if (cer_l)
		retlen += SEPLEN + PORT_Strlen(cer_l);

	    ret = (char *) PORT_ZAlloc(1 + retlen);

	    if (cer_cn)  {
		PORT_Strcpy(ret, cer_cn);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_e)	 {
		PORT_Strcat(ret, cer_e);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_ou1) {
		PORT_Strcat(ret, cer_ou1);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_ou2) {
		PORT_Strcat(ret, cer_ou2);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_ou3) {
		PORT_Strcat(ret, cer_ou3);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_o)	 {
		PORT_Strcat(ret, cer_o);
		PORT_Strcat(ret, SEP);
	    }
	    if (cer_l)
		PORT_Strcat(ret, cer_l);

	    
	    *result = ret;
	    *length = PORT_Strlen(ret);
	    CERT_DestroyCertificate(cert);
	    return 0;
	}
	break;

    case jarCertSerial:
	ret = CERT_Hexify (&cert->serialNumber, 1);
	break;

    case jarCertExpires:
	ret = DER_UTCDayToAscii (&cert->validity.notAfter);
	break;

    case jarCertNickname:
	ret = jar_choose_nickname(cert);
	break;

    case jarCertFinger:
	dig = JAR_calculate_digest((char *) cert->derCert.data,
				   cert->derCert.len);
	if (dig) {
	    hexme.len = sizeof (dig->md5);
	    hexme.data = dig->md5;
	    ret = CERT_Hexify (&hexme, 1);
	}
	break;

    default:
	CERT_DestroyCertificate(cert);
	return JAR_ERR_GENERAL;
    }

    *result = ret ? PORT_Strdup(ret) : NULL;
    *length = ret ? PORT_Strlen(ret) : 0;
    CERT_DestroyCertificate(cert);
    return 0;
}









static char *
jar_cert_element(char *name, char *tag, int occ)
{
    if (name && tag) {
	char *s;
	int found = 0;
	while (occ--) {
	    if (PORT_Strstr(name, tag)) {
		name = PORT_Strstr(name, tag) + PORT_Strlen (tag);
		found = 1;
	    } else {
		name = PORT_Strstr(name, "=");
		if (name == NULL) return NULL;
		found = 0;
	    }
	}
	if (!found)
	    return NULL;

	
	name = PORT_Strdup(name);

	
	for (s = name; *s && *s != '='; s++)
	     ;

	
	while (s > name && *s != ',')
	    s--;

	
	*s = 0;
    }
    return name;
}










static char *
jar_choose_nickname(CERTCertificate *cert)
{
    char *cert_cn;
    char *cert_o;
    char *cert_cn_o;
    int cn_o_length;

    
    if (cert->nickname && PORT_Strncmp(cert->nickname, "tmpcert", 7))
	return PORT_Strdup(cert->nickname);

    
    cert_cn = CERT_GetCommonName(&cert->subject);
    if (cert_cn) {
	CERTCertificate *cert1;
	
	cert1 = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), cert_cn);
	if (cert1 == NULL)
	    return cert_cn;
        CERT_DestroyCertificate(cert1); cert1 = NULL;

	
	cert_o = CERT_GetOrgName (&cert->subject);
	
	cn_o_length = PORT_Strlen(cert_cn) + 3 + PORT_Strlen (cert_o) + 20;
	cert_cn_o = (char*)PORT_ZAlloc(cn_o_length);
	PR_snprintf(cert_cn_o, cn_o_length, "%s's %s Certificate",
		    cert_cn, cert_o);

	cert1 = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), cert_cn_o);
	if (cert1 == NULL) {
	    PORT_Free(cert_cn_o);
	    return cert_cn;
	}
	CERT_DestroyCertificate(cert1); cert1 = NULL;
	PORT_Free(cert_cn_o);
    }

    
    return cert->nickname ? PORT_Strdup(cert->nickname) : NULL;
}








int PR_CALLBACK
JAR_stash_cert(JAR *jar, long keylen, void *key)
{
    int result = 0;
    char *nickname;
    CERTCertTrust trust;
    CERTCertDBHandle *certdb;
    CERTCertificate *cert, *newcert;

    cert = jar_get_certificate(jar, keylen, key, &result);
    if (cert == NULL)
	return JAR_ERR_GENERAL;

    if ((certdb = JAR_open_database()) == NULL)
	return JAR_ERR_GENERAL;

    
    nickname = jar_choose_nickname(cert);
    newcert = CERT_FindCertByNickname(certdb, nickname);
    if (newcert && newcert->isperm) {
	
	CERT_DestroyCertificate(newcert);
	JAR_close_database (certdb);
	return 0;
    }
    if (newcert) {
	CERT_DestroyCertificate(cert);
	cert = newcert;
    }
    if (nickname != NULL) {
	PORT_Memset((void *) &trust, 0, sizeof(trust));
	if (CERT_AddTempCertToPerm (cert, nickname, &trust) != SECSuccess) {
	    
	    result = JAR_ERR_GENERAL;
	}
    }
    CERT_DestroyCertificate(cert);
    JAR_close_database(certdb);
    return result;
}










CERTCertificate *
JAR_fetch_cert(long length, void *key)
{
    CERTIssuerAndSN issuerSN;
    CERTCertificate *cert = NULL;
    CERTCertDBHandle *certdb;

    certdb = JAR_open_database();
    if (certdb) {
	unsigned char *keyData = (unsigned char *)key;
	issuerSN.derIssuer.len = (keyData[0] << 8) + keyData[0];
	issuerSN.derIssuer.data = &keyData[2];
	issuerSN.serialNumber.len = length - (2 + issuerSN.derIssuer.len);
	issuerSN.serialNumber.data = &keyData[2+issuerSN.derIssuer.len];
	cert = CERT_FindCertByIssuerAndSN (certdb, &issuerSN);
	JAR_close_database (certdb);
    }
    return cert;
}








static JAR_Digest *
jar_get_mf_digest(JAR *jar, char *pathname)
{
    JAR_Item *it;
    JAR_Digest *dig;
    ZZLink *link;
    ZZList *list = jar->manifest;

    if (ZZ_ListEmpty (list))
	return NULL;

    for (link = ZZ_ListHead (list);
	 !ZZ_ListIterDone (list, link);
	 link = link->next) {
	it = link->thing;
	if (it->type == jarTypeSect
	    && it->pathname && !PORT_Strcmp(it->pathname, pathname)) {
	    dig = (JAR_Digest *) it->data;
	    return dig;
	}
    }
    return NULL;
}








static char *
jar_basename(const char *path)
{
    char *pith, *e, *basename, *ext;

    if (path == NULL)
	return PORT_Strdup("");

    pith = PORT_Strdup(path);
    basename = pith;
    while (1) {
	for (e = basename; *e && *e != '/' && *e != '\\'; e++)
	     ;
	if (*e)
	    basename = ++e;
	else
	    break;
    }

    if ((ext = PORT_Strrchr(basename, '.')) != NULL)
	*ext = 0;

    
    PORT_Strcpy(pith, basename);
    return pith;
}




















static void
jar_catch_bytes(void *arg, const char *buf, unsigned long len)
{
    

}








static int 
jar_validate_pkcs7(JAR *jar, JAR_Signer *signer, char *data, long length)
{

    SEC_PKCS7ContentInfo *cinfo = NULL;
    SEC_PKCS7DecoderContext *dcx;
    PRBool goodSig;
    int status = 0;
    SECItem detdig;

    PORT_Assert( jar != NULL && signer != NULL );

    if (jar == NULL || signer == NULL)
	return JAR_ERR_ORDER;

    signer->valid = JAR_ERR_SIG;

    
    dcx = SEC_PKCS7DecoderStart(jar_catch_bytes, NULL ,
				NULL , jar->mw,
				NULL, NULL, NULL);
    if (dcx == NULL) {
	
	return JAR_ERR_PK7;
    }

    SEC_PKCS7DecoderUpdate (dcx, data, length);
    cinfo = SEC_PKCS7DecoderFinish (dcx);
    if (cinfo == NULL) {
	
	return JAR_ERR_PK7;
    }
    if (SEC_PKCS7ContentIsEncrypted (cinfo)) {
	
	return JAR_ERR_PK7;
    }
    if (SEC_PKCS7ContentIsSigned (cinfo) == PR_FALSE) {
	
	return JAR_ERR_PK7;
    }

    PORT_SetError(0);

    
    detdig.len = SHA1_LENGTH;
    detdig.data = signer->digest->sha1;
    goodSig = SEC_PKCS7VerifyDetachedSignature(cinfo,
					       certUsageObjectSigner,
					       &detdig, HASH_AlgSHA1,
					       PR_FALSE);
    jar_gather_signers(jar, signer, cinfo);
    if (goodSig == PR_TRUE) {
	
	signer->valid = 0;
    } else {
	status = PORT_GetError();
	PORT_Assert( status < 0 );
	if (status >= 0)
	    status = JAR_ERR_SIG;
	jar->valid = status;
	signer->valid = status;
    }
    jar->pkcs7 = PR_TRUE;
    signer->pkcs7 = PR_TRUE;
    SEC_PKCS7DestroyContentInfo(cinfo);
    return status;
}








static int
jar_gather_signers(JAR *jar, JAR_Signer *signer, SEC_PKCS7ContentInfo *cinfo)
{
    int result;
    CERTCertificate *cert;
    CERTCertDBHandle *certdb;
    SEC_PKCS7SignedData *sdp = cinfo->content.signedData;
    SEC_PKCS7SignerInfo **pksigners, *pksigner;

    if (sdp == NULL)
	return JAR_ERR_PK7;

    pksigners = sdp->signerInfos;
    
    if (pksigners == NULL || pksigners [0] == NULL || pksigners [1] != NULL)
	return JAR_ERR_PK7;

    pksigner = *pksigners;
    cert = pksigner->cert;

    if (cert == NULL)
	return JAR_ERR_PK7;

    certdb = JAR_open_database();
    if (certdb == NULL)
	return JAR_ERR_GENERAL;

    result = jar_add_cert(jar, signer, jarTypeSign, cert);
    JAR_close_database (certdb);
    return result;
}








CERTCertDBHandle *
JAR_open_database(void)
{
    return CERT_GetDefaultCertDB();
}








int 
JAR_close_database(CERTCertDBHandle *certdb)
{
    return 0;
}









static CERTCertificate *
jar_get_certificate(JAR *jar, long keylen, void *key, int *result)
{
    int found = 0;
    JAR_Item *it;
    JAR_Cert *fing = NULL;
    JAR_Context *ctx;

    if (jar == NULL) {
	CERTCertificate * cert = JAR_fetch_cert(keylen, key);
	*result = (cert == NULL) ? JAR_ERR_GENERAL : 0;
	return cert;
    }

    ctx = JAR_find (jar, NULL, jarTypeSign);
    while (JAR_find_next (ctx, &it) >= 0) {
	fing = (JAR_Cert *) it->data;
	if (keylen != fing->length)
	    continue;

	PORT_Assert( keylen < 0xFFFF );
	if (!PORT_Memcmp(fing->key, key, keylen)) {
	    found = 1;
	    break;
	}
    }

    JAR_find_end (ctx);
    if (found == 0) {
	*result = JAR_ERR_GENERAL;
	return NULL;
    }

    PORT_Assert(fing != NULL);
    *result = 0;
    


    return fing->cert;
}







static int
jar_signal(int status, JAR *jar, const char *metafile, char *pathname)
{
    char *errstring = JAR_get_error (status);
    if (jar->signal) {
	(*jar->signal) (status, jar, metafile, pathname, errstring);
	return 0;
    }
    return status;
}








int
jar_append(ZZList *list, int type, char *pathname, void *data, size_t size)
{
    JAR_Item *it = PORT_ZNew(JAR_Item);
    ZZLink *entity;

    if (it == NULL)
	goto loser;

    if (pathname) {
	it->pathname = PORT_Strdup(pathname);
	if (it->pathname == NULL)
	    goto loser;
    }

    it->type = (jarType)type;
    it->data = (unsigned char *) data;
    it->size = size;
    entity = ZZ_NewLink (it);
    if (entity) {
	ZZ_AppendLink (list, entity);
	return 0;
    }

loser:
    if (it) {
	if (it->pathname) 
	    PORT_Free(it->pathname);
	PORT_Free(it);
    }
    return JAR_ERR_MEMORY;
}
