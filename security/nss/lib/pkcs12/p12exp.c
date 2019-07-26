



#include "plarena.h"
#include "secitem.h"
#include "secoid.h"
#include "seccomon.h"
#include "secport.h"
#include "cert.h"
#include "pkcs12.h"
#include "p12local.h"
#include "secpkcs7.h"
#include "secasn1.h"
#include "secerr.h"
#include "p12plcy.h"


static void
sec_pkcs12_destroy_nickname_list(SECItem **nicknames)
{
    int i = 0;

    if(nicknames == NULL) {
	return;
    }

    while(nicknames[i] != NULL) {
	SECITEM_FreeItem(nicknames[i], PR_FALSE);
	i++;
    }

    PORT_Free(nicknames);
}
   
 
static void
sec_pkcs12_destroy_certificate_list(CERTCertificate **ref_certs)
{
    int i = 0;

    if(ref_certs == NULL) {
	return;
    }

    while(ref_certs[i] != NULL) {
	CERT_DestroyCertificate(ref_certs[i]);
	i++;
    }
}

static void
sec_pkcs12_destroy_cinfos_for_cert_bags(SEC_PKCS12CertAndCRLBag *certBag)
{
    int j = 0;
    j = 0;
    while(certBag->certAndCRLs[j] != NULL) {
	SECOidTag certType = SECOID_FindOIDTag(&certBag->certAndCRLs[j]->BagID);
	if(certType == SEC_OID_PKCS12_X509_CERT_CRL_BAG) {
	    SEC_PKCS12X509CertCRL *x509;
	    x509 = certBag->certAndCRLs[j]->value.x509;
	    SEC_PKCS7DestroyContentInfo(&x509->certOrCRL);
	}
	j++;
    }
}




static void
sec_pkcs12_destroy_cert_content_infos(SEC_PKCS12SafeContents *safe,
				      SEC_PKCS12Baggage *baggage)
{
    int i, j;

    if((safe != NULL) && (safe->contents != NULL)) {
	i = 0;
	while(safe->contents[i] != NULL) {
	    SECOidTag bagType = SECOID_FindOIDTag(&safe->contents[i]->safeBagType);
	    if(bagType == SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID) {
		SEC_PKCS12CertAndCRLBag *certBag;
		certBag = safe->contents[i]->safeContent.certAndCRLBag;
		sec_pkcs12_destroy_cinfos_for_cert_bags(certBag);
	    }
	    i++;
	}
    }

    if((baggage != NULL) && (baggage->bags != NULL)) {
	i = 0;
	while(baggage->bags[i] != NULL) { 
	    if(baggage->bags[i]->unencSecrets != NULL) {
		j = 0;
		while(baggage->bags[i]->unencSecrets[j] != NULL) {
		    SECOidTag bagType;
		    bagType = SECOID_FindOIDTag(&baggage->bags[i]->unencSecrets[j]->safeBagType);
		    if(bagType == SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID) {
			SEC_PKCS12CertAndCRLBag *certBag;
			certBag = baggage->bags[i]->unencSecrets[j]->safeContent.certAndCRLBag;
			sec_pkcs12_destroy_cinfos_for_cert_bags(certBag);
		    }
		    j++;
		}
	    }
	    i++;
	}
    }
}




static SECItem **
sec_pkcs12_convert_nickname_list(char **nicknames)
{
    SECItem **nicks;
    int i, j;
    PRBool error = PR_FALSE;

    if(nicknames == NULL) {
	return NULL;
    }

    i = j = 0;
    while(nicknames[i] != NULL) {
	i++;
    }

    	
    nicks = (SECItem **)PORT_ZAlloc(sizeof(SECItem *) * (i + 1));
    if(nicks != NULL) {
	for(j = 0; ((j < i) && (error == PR_FALSE)); j++) {
	    nicks[j] = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
	    if(nicks[j] != NULL) {
		nicks[j]->data = 
		    (unsigned char *)PORT_ZAlloc(PORT_Strlen(nicknames[j])+1);
		if(nicks[j]->data != NULL) {
		    nicks[j]->len = PORT_Strlen(nicknames[j]);
		    PORT_Memcpy(nicks[j]->data, nicknames[j], nicks[j]->len);
		    nicks[j]->data[nicks[j]->len] = 0;
		} else {
		    error = PR_TRUE;
		}
	    } else {
	       error = PR_TRUE;
	    }
	}
    }

    if(error == PR_TRUE) {
        for(i = 0; i < j; i++) { 
	    SECITEM_FreeItem(nicks[i], PR_TRUE);
	}
	PORT_Free(nicks);
	nicks = NULL;
    }

    return nicks;
}









static SEC_PKCS12CertAndCRL *
sec_pkcs12_get_cert(PRArenaPool *poolp,
		       CERTCertificate *add_cert, 
		       SECItem *nickname)
{
    SEC_PKCS12CertAndCRL *cert;
    SEC_PKCS7ContentInfo *cinfo;
    SGNDigestInfo *t_di;
    void *mark;
    SECStatus rv;

    if((poolp == NULL) || (add_cert == NULL) || (nickname == NULL)) {
    	return NULL;
    }
    mark = PORT_ArenaMark(poolp);

    cert = sec_pkcs12_new_cert_crl(poolp, SEC_OID_PKCS12_X509_CERT_CRL_BAG);
    if(cert != NULL) {

	
	rv = SECITEM_CopyItem(poolp, &cert->nickname, nickname);
	if(rv != SECSuccess) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    cert = NULL;
	} else {

	    



	    cinfo = SEC_PKCS7CreateCertsOnly(add_cert, PR_TRUE, NULL);
	    rv = SEC_PKCS7PrepareForEncode(cinfo, NULL, NULL, NULL);

	    
	    if((cinfo != NULL) && (rv == SECSuccess))
	    {
		PORT_Memcpy(&cert->value.x509->certOrCRL, cinfo, sizeof(*cinfo));
		t_di = sec_pkcs12_compute_thumbprint(&add_cert->derCert);
		if(t_di != NULL)
		{
		    
		    rv = SGN_CopyDigestInfo(poolp, &cert->value.x509->thumbprint,
		    			    t_di);
		    if(rv != SECSuccess) {
			cert = NULL;
			PORT_SetError(SEC_ERROR_NO_MEMORY);
		    }
		    SGN_DestroyDigestInfo(t_di);
		}
		else
		    cert = NULL;
	    }
	}
    }

    if (cert == NULL) {
	PORT_ArenaRelease(poolp, mark);
    } else {
	PORT_ArenaUnmark(poolp, mark);
    }

    return cert;
}









static SEC_PKCS12PrivateKey *
sec_pkcs12_get_private_key(PRArenaPool *poolp,
			   SECItem *nickname,
			   CERTCertificate *cert,
			   void *wincx)
{
    SECKEYPrivateKeyInfo *pki;
    SEC_PKCS12PrivateKey *pk;
    SECStatus rv;
    void *mark;

    if((poolp == NULL) || (nickname == NULL)) {
	return NULL;
    }

    mark = PORT_ArenaMark(poolp);

    
    pki = PK11_ExportPrivateKeyInfo(nickname, cert, wincx);
    if(pki == NULL) {
	PORT_ArenaRelease(poolp, mark);
	PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY);
	return NULL;
    }

    pk = (SEC_PKCS12PrivateKey *)PORT_ArenaZAlloc(poolp,
						  sizeof(SEC_PKCS12PrivateKey));
    if(pk != NULL) {
	rv = sec_pkcs12_init_pvk_data(poolp, &pk->pvkData);

	if(rv == SECSuccess) {
	    
	    rv = SECKEY_CopyPrivateKeyInfo(poolp, &pk->pkcs8data, pki);
	    if(rv == SECSuccess) {
		rv = SECITEM_CopyItem(poolp, &pk->pvkData.nickname, nickname);
	    }
	}

	if(rv != SECSuccess) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    pk = NULL;
	}
    } else {
	PORT_SetError(SEC_ERROR_NO_MEMORY); 
    }

    
    SECKEY_DestroyPrivateKeyInfo(pki, PR_TRUE);
    if (pk == NULL) {
	PORT_ArenaRelease(poolp, mark);
    } else {
	PORT_ArenaUnmark(poolp, mark);
    }

    return pk;
}









static SEC_PKCS12ESPVKItem *
sec_pkcs12_get_shrouded_key(PRArenaPool *poolp,
			    SECItem *nickname,
			    CERTCertificate *cert,
			    SECOidTag algorithm, 
			    SECItem *pwitem,
			    PKCS12UnicodeConvertFunction unicodeFn,
			    void *wincx)
{
    SECKEYEncryptedPrivateKeyInfo *epki;
    SEC_PKCS12ESPVKItem *pk;
    void *mark;
    SECStatus rv;
    PK11SlotInfo *slot = NULL;
    PRBool swapUnicodeBytes = PR_FALSE;

#ifdef IS_LITTLE_ENDIAN
    swapUnicodeBytes = PR_TRUE;
#endif

    if((poolp == NULL) || (nickname == NULL))
	return NULL;

    mark = PORT_ArenaMark(poolp);

    
    slot = PK11_GetInternalKeySlot();

    
    epki = PK11_ExportEncryptedPrivateKeyInfo(slot, algorithm, pwitem, 
    					      nickname, cert, 1, 0, NULL);
    PK11_FreeSlot(slot);
    if(epki == NULL) {
	PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }

    
    pk = sec_pkcs12_create_espvk(poolp, SEC_OID_PKCS12_PKCS8_KEY_SHROUDING);
    if(pk != NULL) {
	rv = sec_pkcs12_init_pvk_data(poolp, &pk->espvkData);
	rv = SECITEM_CopyItem(poolp, &pk->espvkData.nickname, nickname);
	pk->espvkCipherText.pkcs8KeyShroud = 
	    (SECKEYEncryptedPrivateKeyInfo *)PORT_ArenaZAlloc(poolp,
					sizeof(SECKEYEncryptedPrivateKeyInfo));
	if((pk->espvkCipherText.pkcs8KeyShroud != NULL)  && (rv == SECSuccess)) {
	    rv = SECKEY_CopyEncryptedPrivateKeyInfo(poolp, 
					pk->espvkCipherText.pkcs8KeyShroud, epki);
	    if(rv == SECSuccess) {
		rv = (*unicodeFn)(poolp, &pk->espvkData.uniNickName, nickname, 
				  PR_TRUE, swapUnicodeBytes);
	    }
	}

	if(rv != SECSuccess) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    pk = NULL;
	}
    }

    SECKEY_DestroyEncryptedPrivateKeyInfo(epki, PR_TRUE);
    if(pk == NULL) {
	PORT_ArenaRelease(poolp, mark);
    } else {
	PORT_ArenaUnmark(poolp, mark);
    }
	
    return pk;
}






static SECStatus 
sec_pkcs12_add_thumbprint(SEC_PKCS12PVKSupportingData *pvk,
			  SGNDigestInfo *thumb)
{
    SGNDigestInfo **thumb_list = NULL;
    int nthumbs, size;
    void *mark, *dummy;
    SECStatus rv = SECFailure;

    if((pvk == NULL) || (thumb == NULL)) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(pvk->poolp);

    thumb_list = pvk->assocCerts;
    nthumbs = pvk->nThumbs;

    


    size = sizeof(SGNDigestInfo *);
    dummy = PORT_ArenaGrow(pvk->poolp, thumb_list, (size * (nthumbs + 1)),
    			   (size * (nthumbs + 2)));
    thumb_list = dummy;
    if(dummy != NULL) {
	thumb_list[nthumbs] = (SGNDigestInfo *)PORT_ArenaZAlloc(pvk->poolp, 
						sizeof(SGNDigestInfo));
	if(thumb_list[nthumbs] != NULL) {
	    SGN_CopyDigestInfo(pvk->poolp, thumb_list[nthumbs], thumb);
	    nthumbs += 1;
	    thumb_list[nthumbs] = 0;
	} else {
	    dummy = NULL;
	}
    }

    if(dummy == NULL) {
    	PORT_ArenaRelease(pvk->poolp, mark);
	return SECFailure;
    } 

    pvk->assocCerts = thumb_list;
    pvk->nThumbs = nthumbs;

    PORT_ArenaUnmark(pvk->poolp, mark);
    return SECSuccess;
}





static SEC_PKCS12ESPVKItem *
sec_pkcs12_get_espvk_by_name(SEC_PKCS12Baggage *luggage, 
			     SECItem *name)
{
    PRBool found = PR_FALSE;
    SEC_PKCS12ESPVKItem *espvk = NULL;
    int i, j;
    SECComparison rv = SECEqual;
    SECItem *t_name;
    SEC_PKCS12BaggageItem *bag;

    if((luggage == NULL) || (name == NULL)) {
	return NULL;
    }

    i = 0;
    while((found == PR_FALSE) && (i < luggage->luggage_size)) {
	j = 0;
	bag = luggage->bags[i];
	while((found == PR_FALSE) && (j < bag->nEspvks)) {
	    espvk = bag->espvks[j];
	    if(espvk->poolp == NULL) {
		espvk->poolp = luggage->poolp;
	    }
	    t_name = SECITEM_DupItem(&espvk->espvkData.nickname);
	    if(t_name != NULL) {
		rv = SECITEM_CompareItem(name, t_name);
		if(rv == SECEqual) {
		    found = PR_TRUE;
		}
		SECITEM_FreeItem(t_name, PR_TRUE);
	    } else {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		return NULL;
	    }
	    j++;
	}
	i++;
    }

    if(found != PR_TRUE) {
	PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME);
	return NULL;
    }

    return espvk;
}




static SECStatus 
sec_pkcs12_propagate_thumbprints(SECItem **nicknames,
				 CERTCertificate **ref_certs,
				 SEC_PKCS12SafeContents *safe,
				 SEC_PKCS12Baggage *baggage)
{
    SEC_PKCS12CertAndCRL *cert;
    SEC_PKCS12PrivateKey *key;
    SEC_PKCS12ESPVKItem *espvk;
    int i;
    PRBool error = PR_FALSE;
    SECStatus rv = SECFailure;

    if((nicknames == NULL) || (safe == NULL)) {
	return SECFailure;
    }

    i = 0;
    while((nicknames[i] != NULL) && (error == PR_FALSE)) {
	
	cert = (SEC_PKCS12CertAndCRL *)sec_pkcs12_find_object(safe, baggage,
					SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID,
					nicknames[i], NULL);
	if(cert != NULL) {
	    
	    key = (SEC_PKCS12PrivateKey *)sec_pkcs12_find_object(safe, baggage,
	    				SEC_OID_PKCS12_KEY_BAG_ID,
	    				nicknames[i], NULL);
	    if(key != NULL) {
		key->pvkData.poolp = key->poolp;
		rv = sec_pkcs12_add_thumbprint(&key->pvkData, 
			&cert->value.x509->thumbprint);
		if(rv == SECFailure)
		    error = PR_TRUE;  
	    }

	    
	    if((baggage != NULL) && (error == PR_FALSE)) {
		espvk = sec_pkcs12_get_espvk_by_name(baggage, nicknames[i]);
		if(espvk != NULL) {
		    espvk->espvkData.poolp = espvk->poolp;
		    rv = sec_pkcs12_add_thumbprint(&espvk->espvkData,
			&cert->value.x509->thumbprint);
		    if(rv == SECFailure)
			error = PR_TRUE;  
		}
	    }
	}
	i++;
    }

    if(error == PR_TRUE) {
	return SECFailure;
    }

    return SECSuccess;
}


SECStatus 
sec_pkcs12_append_safe_bag(SEC_PKCS12SafeContents *safe,
			   SEC_PKCS12SafeBag *bag)
{
    int size;
    void *mark = NULL, *dummy = NULL;

    if((bag == NULL) || (safe == NULL))
	return SECFailure;

    mark = PORT_ArenaMark(safe->poolp);

    size = (safe->safe_size * sizeof(SEC_PKCS12SafeBag *));
    
    if(safe->safe_size > 0) {
	dummy = (SEC_PKCS12SafeBag **)PORT_ArenaGrow(safe->poolp, 
	    				safe->contents, 
	    				size, 
	    				(size + sizeof(SEC_PKCS12SafeBag *)));
	safe->contents = dummy;
    } else {
	safe->contents = (SEC_PKCS12SafeBag **)PORT_ArenaZAlloc(safe->poolp, 
	    (2 * sizeof(SEC_PKCS12SafeBag *)));
	dummy = safe->contents;
    }

    if(dummy == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    safe->contents[safe->safe_size] = bag;
    safe->safe_size++;
    safe->contents[safe->safe_size] = NULL;

    PORT_ArenaUnmark(safe->poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(safe->poolp, mark);
    return SECFailure;
}


static SECStatus 
sec_pkcs12_append_cert_to_bag(PRArenaPool *arena,
			      SEC_PKCS12SafeBag *safebag,
			      CERTCertificate *cert,
			      SECItem *nickname)
{
    int size;
    void *dummy = NULL, *mark = NULL;
    SEC_PKCS12CertAndCRL *p12cert;
    SEC_PKCS12CertAndCRLBag *bag;

    if((arena == NULL) || (safebag == NULL) || 
    	(cert == NULL) || (nickname == NULL)) {
	return SECFailure;
    }

    bag = safebag->safeContent.certAndCRLBag;
    if(bag == NULL) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(arena);

    p12cert = sec_pkcs12_get_cert(arena, cert, nickname);
    if(p12cert == NULL) {
	PORT_ArenaRelease(bag->poolp, mark);
	return SECFailure;
    }

    size = bag->bag_size * sizeof(SEC_PKCS12CertAndCRL *);
    if(bag->bag_size > 0) {
	dummy = (SEC_PKCS12CertAndCRL **)PORT_ArenaGrow(bag->poolp,
	    bag->certAndCRLs, size, size + sizeof(SEC_PKCS12CertAndCRL *));
	bag->certAndCRLs = dummy;
    } else {
	bag->certAndCRLs = (SEC_PKCS12CertAndCRL **)PORT_ArenaZAlloc(bag->poolp,
	    (2 * sizeof(SEC_PKCS12CertAndCRL *)));
	dummy = bag->certAndCRLs;
    }

    if(dummy == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    bag->certAndCRLs[bag->bag_size] = p12cert;
    bag->bag_size++;
    bag->certAndCRLs[bag->bag_size] = NULL;

    PORT_ArenaUnmark(bag->poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(bag->poolp, mark);
    return SECFailure;
}


SECStatus 
sec_pkcs12_append_key_to_bag(SEC_PKCS12SafeBag *safebag,
			     SEC_PKCS12PrivateKey *pk)
{
    void *mark, *dummy;
    SEC_PKCS12PrivateKeyBag *bag;
    int size;

    if((safebag == NULL) || (pk == NULL))
	return SECFailure;

    bag = safebag->safeContent.keyBag;
    if(bag == NULL) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(bag->poolp);

    size = (bag->bag_size * sizeof(SEC_PKCS12PrivateKey *));

    if(bag->bag_size > 0) {
	dummy = (SEC_PKCS12PrivateKey **)PORT_ArenaGrow(bag->poolp,
					bag->privateKeys, 
					size, 
					size + sizeof(SEC_PKCS12PrivateKey *));
	bag->privateKeys = dummy;
    } else {
	bag->privateKeys = (SEC_PKCS12PrivateKey **)PORT_ArenaZAlloc(bag->poolp,
	    (2 * sizeof(SEC_PKCS12PrivateKey *)));
	dummy = bag->privateKeys;
    }

    if(dummy == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    bag->privateKeys[bag->bag_size] = pk;
    bag->bag_size++;
    bag->privateKeys[bag->bag_size] = NULL;

    PORT_ArenaUnmark(bag->poolp, mark);
    return SECSuccess;

loser:
    
    PORT_ArenaRelease(bag->poolp, mark);
    return SECFailure;
}


static SECStatus 
sec_pkcs12_append_unshrouded_bag(SEC_PKCS12BaggageItem *bag,
				 SEC_PKCS12SafeBag *u_bag)
{
    int size;
    void *mark = NULL, *dummy = NULL;

    if((bag == NULL) || (u_bag == NULL))
	return SECFailure;

    mark = PORT_ArenaMark(bag->poolp);

    
    size = (bag->nSecrets + 1) * sizeof(SEC_PKCS12SafeBag *);
    dummy = PORT_ArenaGrow(bag->poolp,
	    		bag->unencSecrets, size, 
	    		size + sizeof(SEC_PKCS12SafeBag *));
    bag->unencSecrets = dummy;
    if(dummy == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    bag->unencSecrets[bag->nSecrets] = u_bag;
    bag->nSecrets++;
    bag->unencSecrets[bag->nSecrets] = NULL;

    PORT_ArenaUnmark(bag->poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(bag->poolp, mark);
    return SECFailure;
}












static SECStatus
sec_pkcs12_package_certs_and_keys(SECItem **nicknames,
				  CERTCertificate **ref_certs,
				  PRBool unencryptedCerts,
				  SEC_PKCS12SafeContents **rSafe,
				  SEC_PKCS12Baggage **rBaggage,
				  PRBool shroud_keys, 
				  SECOidTag shroud_alg,
				  SECItem *pwitem,
				  PKCS12UnicodeConvertFunction unicodeFn,
				  void *wincx)
{
    PRArenaPool *permArena;
    SEC_PKCS12SafeContents *safe = NULL;
    SEC_PKCS12Baggage *baggage = NULL;

    SECStatus rv = SECFailure;
    PRBool problem = PR_FALSE;

    SEC_PKCS12ESPVKItem *espvk = NULL;
    SEC_PKCS12PrivateKey *pk = NULL;
    CERTCertificate *add_cert = NULL;
    SEC_PKCS12SafeBag *certbag = NULL, *keybag = NULL;
    SEC_PKCS12BaggageItem *external_bag = NULL;
    int ncerts = 0, nkeys = 0;
    int i;

    if((nicknames == NULL) || (rSafe == NULL) || (rBaggage == NULL)) {
	return SECFailure;
    }
	
    *rBaggage = baggage;
    *rSafe = safe;

    permArena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(permArena == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
     }

    
    safe = sec_pkcs12_create_safe_contents(permArena);
    if(safe == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	rv = SECFailure;
	goto loser;
    }

    certbag = sec_pkcs12_create_safe_bag(permArena, 
					 SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID);
    if(certbag == NULL) {
	rv = SECFailure;
	goto loser;
    }

    if(shroud_keys != PR_TRUE) {
	keybag = sec_pkcs12_create_safe_bag(permArena, 
					    SEC_OID_PKCS12_KEY_BAG_ID);
	if(keybag == NULL) {
	    rv = SECFailure;
	    goto loser;
	}
    }

    if((shroud_keys == PR_TRUE) || (unencryptedCerts == PR_TRUE)) {
	baggage = sec_pkcs12_create_baggage(permArena);
    	if(baggage == NULL) {
	    rv = SECFailure;
	    goto loser;
	}
	external_bag = sec_pkcs12_create_external_bag(baggage);
    }

    
    i = 0;
    while((nicknames[i] != NULL) && (problem == PR_FALSE)) {
	if(ref_certs[i] != NULL) {
	    
	    rv = sec_pkcs12_append_cert_to_bag(permArena, certbag, 
	    				       ref_certs[i],
	    				       nicknames[i]);
	    if(rv == SECFailure) {
		problem = PR_FALSE;
	    } else {
		ncerts++;
	    }

	    if(rv == SECSuccess) {
		
		if(shroud_keys == PR_TRUE) {
	    	    espvk = sec_pkcs12_get_shrouded_key(permArena, 
							nicknames[i],
	    	    					ref_certs[i],
							shroud_alg, 
							pwitem, unicodeFn,
							wincx);
		    if(espvk != NULL) {
			rv = sec_pkcs12_append_shrouded_key(external_bag, espvk);
			SECITEM_CopyItem(permArena, &espvk->derCert,
					 &ref_certs[i]->derCert);
		    } else {
			rv = SECFailure;
		    }
		} else {
		    pk = sec_pkcs12_get_private_key(permArena, nicknames[i], 
		    				    ref_certs[i], wincx);
		    if(pk != NULL) {
			rv = sec_pkcs12_append_key_to_bag(keybag, pk);
			SECITEM_CopyItem(permArena, &espvk->derCert,
					 &ref_certs[i]->derCert);
		    } else {
			rv = SECFailure;
		    }
		}
	
		if(rv == SECFailure) {
		    problem = PR_TRUE;
		} else {
		    nkeys++;
		}
	    }
	} else {
	    
	    problem = PR_TRUE;
	}
	i++;
    }

    
loser:
    if(problem == PR_FALSE) {
	


	if(ncerts > 0) {
	    if(unencryptedCerts != PR_TRUE) {
		rv = sec_pkcs12_append_safe_bag(safe, certbag);
	    } else {
		rv = sec_pkcs12_append_unshrouded_bag(external_bag, certbag);
	    }
	} else {
	    rv = SECSuccess;
	}

	
	if((rv == SECSuccess) && (shroud_keys == PR_FALSE) && (nkeys > 0)) {
	    rv = sec_pkcs12_append_safe_bag(safe, keybag);
	}
    } else {
	rv = SECFailure;
    }

    
    if((shroud_keys == PR_TRUE) || (unencryptedCerts == PR_TRUE)) {
	if(((unencryptedCerts == PR_TRUE) && (ncerts == 0)) &&
	   ((shroud_keys == PR_TRUE) && (nkeys == 0)))
		baggage = NULL;
    } else {
	baggage = NULL;
    }

    if((problem == PR_TRUE) || (rv == SECFailure)) {
	PORT_FreeArena(permArena, PR_TRUE);
	rv = SECFailure;
	baggage = NULL;
	safe = NULL;
    }

    *rBaggage = baggage;
    *rSafe = safe;

    return rv;
}




static SECItem *
sec_pkcs12_encode_safe_contents(SEC_PKCS12SafeContents *safe)
{
    SECItem *dsafe = NULL, *tsafe;
    void *dummy = NULL;
    PRArenaPool *arena;

    if(safe == NULL) {
	return NULL;
    }







    arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(arena == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    tsafe = (SECItem *)PORT_ArenaZAlloc(arena, sizeof(SECItem));
    if(tsafe != NULL) {
	dummy = SEC_ASN1EncodeItem(arena, tsafe, safe, 
	    SEC_PKCS12SafeContentsTemplate);
	if(dummy != NULL) {
	    dsafe = SECITEM_DupItem(tsafe);
	} else {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	}
    } else {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena(arena, PR_TRUE);

    return dsafe;
}















static SEC_PKCS7ContentInfo *
sec_pkcs12_get_auth_safe(SEC_PKCS12SafeContents *safe, 
			 SEC_PKCS12Baggage *baggage,  
			 SECOidTag algorithm, 
			 SECItem *pwitem,
			 PKCS12UnicodeConvertFunction unicodeFn,
			 void *wincx)
{
    SECItem *src = NULL, *dest = NULL, *psalt = NULL;
    PRArenaPool *poolp;
    SEC_PKCS12AuthenticatedSafe *asafe;
    SEC_PKCS7ContentInfo *safe_cinfo = NULL;
    SEC_PKCS7ContentInfo *asafe_cinfo = NULL;
    void *dummy;
    SECStatus rv = SECSuccess;
    PRBool swapUnicodeBytes = PR_FALSE;

#ifdef IS_LITTLE_ENDIAN
    swapUnicodeBytes = PR_TRUE;
#endif
    
    if(((safe != NULL) && (pwitem == NULL)) && (baggage == NULL))
	return NULL;

    poolp = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(poolp == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    
    asafe = sec_pkcs12_new_asafe(poolp);
    if(asafe != NULL) {

	
	dummy = SEC_ASN1EncodeInteger(asafe->poolp, &asafe->version,
				      SEC_PKCS12_PFX_VERSION);
	if(dummy == NULL) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    rv = SECFailure;
	    goto loser;
	}

	
	psalt = sec_pkcs12_generate_salt();
	if(psalt != NULL) {
	    rv = SECITEM_CopyItem(asafe->poolp, &asafe->privacySalt,
				  psalt);
	    if(rv == SECSuccess) {
		asafe->privacySalt.len *= 8;
	    } 
	    else {
		SECITEM_ZfreeItem(psalt, PR_TRUE);
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    }
	} 

	if((psalt == NULL) || (rv == SECFailure)) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    rv = SECFailure;
	    goto loser;
	}

	
	if(safe != NULL) 
	{
	    safe_cinfo = SEC_PKCS7CreateEncryptedData(algorithm, NULL, wincx);
	    if((safe_cinfo != NULL) && (safe->safe_size > 0)) {
		


		src = sec_pkcs12_encode_safe_contents(safe);

		if(src != NULL) {
		    rv = SEC_PKCS7SetContent(safe_cinfo, (char *)src->data, src->len);
		    SECITEM_ZfreeItem(src, PR_TRUE);
		    if(rv == SECSuccess) {
			SECItem *vpwd;
			vpwd = sec_pkcs12_create_virtual_password(pwitem, psalt,
						unicodeFn, swapUnicodeBytes);
			if(vpwd != NULL) {
			    rv = SEC_PKCS7EncryptContents(NULL, safe_cinfo,
							  vpwd, wincx);
			    SECITEM_ZfreeItem(vpwd, PR_TRUE);
			} else {
			    rv = SECFailure;
			    PORT_SetError(SEC_ERROR_NO_MEMORY);
			}
		    } else {
			PORT_SetError(SEC_ERROR_NO_MEMORY);
		    }
		} else {
		    rv = SECFailure;
		}
	    } else if(safe->safe_size > 0) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    } else {
	    	
		rv = SEC_PKCS7SetContent(safe_cinfo, NULL, 0);
		if(rv != SECFailure) {
		    PORT_SetError(SEC_ERROR_NO_MEMORY);
		}
	    }

	    if(rv != SECSuccess) {
		SEC_PKCS7DestroyContentInfo(safe_cinfo);
		safe_cinfo = NULL;
		goto loser;
	    }

	    asafe->safe = safe_cinfo;	
	    


	}

	
	if(baggage != NULL) {
	    PORT_Memcpy(&asafe->baggage, baggage, sizeof(*baggage));
	}

	
	dest = (SECItem *)PORT_ArenaZAlloc(poolp, sizeof(SECItem));
	if(dest != NULL) {
	    dummy = SEC_ASN1EncodeItem(poolp, dest, asafe, 
				SEC_PKCS12AuthenticatedSafeTemplate);
	    if(dummy != NULL) {
		asafe_cinfo = SEC_PKCS7CreateData();
		if(asafe_cinfo != NULL) {
		    rv = SEC_PKCS7SetContent(asafe_cinfo, 
					 	(char *)dest->data, 
						dest->len);
		    if(rv != SECSuccess) {
			PORT_SetError(SEC_ERROR_NO_MEMORY);
			SEC_PKCS7DestroyContentInfo(asafe_cinfo);
			asafe_cinfo = NULL;
		    }
		}
	    } else {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		rv = SECFailure;
	    }
	}
    }

loser:
    PORT_FreeArena(poolp, PR_TRUE);
    if(safe_cinfo != NULL) {
    	SEC_PKCS7DestroyContentInfo(safe_cinfo);
    }
    if(psalt != NULL) {
	SECITEM_ZfreeItem(psalt, PR_TRUE);
    }

    if(rv == SECFailure) {
	return NULL;
    }
	
    return asafe_cinfo;
}




static SEC_PKCS12PFXItem *
sec_pkcs12_get_pfx(SEC_PKCS7ContentInfo *cinfo, 
		   PRBool do_mac, 
		   SECItem *pwitem, PKCS12UnicodeConvertFunction unicodeFn)
{
    SECItem *dest = NULL, *mac = NULL, *salt = NULL, *key = NULL;
    SEC_PKCS12PFXItem *pfx;
    SECStatus rv = SECFailure;
    SGNDigestInfo *di;
    SECItem *vpwd;
    PRBool swapUnicodeBytes = PR_FALSE;

#ifdef IS_LITTLE_ENDIAN
    swapUnicodeBytes = PR_TRUE;
#endif

    if((cinfo == NULL) || ((do_mac == PR_TRUE) && (pwitem == NULL))) {
	return NULL;
    }

    
    pfx = sec_pkcs12_new_pfx();
    if(pfx == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    PORT_Memcpy(&pfx->authSafe, cinfo, sizeof(*cinfo));
    if(do_mac == PR_TRUE) {

    	
	salt = sec_pkcs12_generate_salt();
	if(salt != NULL) {
	    rv = SECITEM_CopyItem(pfx->poolp, &pfx->macData.macSalt, salt);
	    pfx->macData.macSalt.len *= 8;

	    vpwd = sec_pkcs12_create_virtual_password(pwitem, salt,
						unicodeFn, swapUnicodeBytes);
	    if(vpwd == NULL) {
		rv = SECFailure;
		key = NULL;
	    } else {
		key = sec_pkcs12_generate_key_from_password(SEC_OID_SHA1,
							salt, vpwd);
		SECITEM_ZfreeItem(vpwd, PR_TRUE);
	    }

	    if((key != NULL) && (rv == SECSuccess)) {
		dest = SEC_PKCS7GetContent(cinfo);
		if(dest != NULL) {

		    
		    mac = sec_pkcs12_generate_mac(key, dest, PR_FALSE);
		    if(mac != NULL) {
			di = SGN_CreateDigestInfo(SEC_OID_SHA1, 
			    			  mac->data, mac->len);
			if(di != NULL) {
			    rv = SGN_CopyDigestInfo(pfx->poolp, 
						    &pfx->macData.safeMac, di);
			    SGN_DestroyDigestInfo(di);
			} else {
			    PORT_SetError(SEC_ERROR_NO_MEMORY);
			}
			SECITEM_ZfreeItem(mac, PR_TRUE);
		    }
		} else {
		    rv = SECFailure;
		}
	    } else {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		rv = SECFailure;
	    }

	    if(key != NULL) {
		SECITEM_ZfreeItem(key, PR_TRUE);
	    }
	    SECITEM_ZfreeItem(salt, PR_TRUE);
	}
    }

    if(rv == SECFailure) {
	SEC_PKCS12DestroyPFX(pfx);
	pfx = NULL;
    }

    return pfx;
}


static SECItem *
sec_pkcs12_encode_pfx(SEC_PKCS12PFXItem *pfx)
{
    SECItem *dest;
    void *dummy;

    if(pfx == NULL) {
	return NULL;
    }

    dest = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
    if(dest == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    dummy = SEC_ASN1EncodeItem(NULL, dest, pfx, SEC_PKCS12PFXItemTemplate);
    if(dummy == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	SECITEM_ZfreeItem(dest, PR_TRUE);
	dest = NULL;
    }

    return dest;
}

SECItem *
SEC_PKCS12GetPFX(char **nicknames,
		 CERTCertificate **ref_certs,
		 PRBool shroud_keys, 
		 SEC_PKCS5GetPBEPassword pbef, 
		 void *pbearg,
		 PKCS12UnicodeConvertFunction unicodeFn,
		 void *wincx)
{
    SECItem **nicks = NULL;
    SEC_PKCS12PFXItem *pfx = NULL;
    SEC_PKCS12Baggage *baggage = NULL;
    SEC_PKCS12SafeContents *safe = NULL;
    SEC_PKCS7ContentInfo *cinfo = NULL;
    SECStatus rv = SECFailure;
    SECItem *dest = NULL, *pwitem = NULL;
    PRBool problem = PR_FALSE;
    PRBool unencryptedCerts;
    SECOidTag shroud_alg, safe_alg;

    
    unencryptedCerts = !SEC_PKCS12IsEncryptionAllowed();
    if(!unencryptedCerts) {
	safe_alg = SEC_PKCS12GetPreferredEncryptionAlgorithm();
	if(safe_alg == SEC_OID_UNKNOWN) {
	    safe_alg = SEC_PKCS12GetStrongestAllowedAlgorithm();
	}
	if(safe_alg == SEC_OID_UNKNOWN) {
	    unencryptedCerts = PR_TRUE;
	    



	    safe_alg = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC;
	}
    } else {
	



	safe_alg = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC;
    }

    
    shroud_alg = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC;

    
    if(PK11_IsFIPS() && !unencryptedCerts) {
	unencryptedCerts = PR_TRUE;
    }

    if((nicknames == NULL) || (pbef == NULL) || (ref_certs == NULL)) {
	problem = PR_TRUE;
	goto loser;
    }


    
    pwitem = (*pbef)(pbearg);
    if(pwitem == NULL) {
	problem = PR_TRUE;
	goto loser;
    }
    nicks = sec_pkcs12_convert_nickname_list(nicknames);

    
    rv = sec_pkcs12_package_certs_and_keys(nicks, ref_certs, unencryptedCerts,
    					   &safe, &baggage, shroud_keys,
    					   shroud_alg, pwitem, unicodeFn, wincx);
    if(rv == SECFailure) {
	problem = PR_TRUE;
    }

    if((safe != NULL) && (problem == PR_FALSE)) {
	
	rv = sec_pkcs12_propagate_thumbprints(nicks, ref_certs, safe, baggage);

	
	cinfo = sec_pkcs12_get_auth_safe(safe, baggage, 
					 safe_alg, pwitem, unicodeFn, wincx);

	sec_pkcs12_destroy_cert_content_infos(safe, baggage);

	
	if(cinfo != NULL) {
	    pfx = sec_pkcs12_get_pfx(cinfo, PR_TRUE, pwitem, unicodeFn);
	    if(pfx != NULL) {
		dest = sec_pkcs12_encode_pfx(pfx);
		SEC_PKCS12DestroyPFX(pfx);
	    }
	    SEC_PKCS7DestroyContentInfo(cinfo);
	}

	if(safe != NULL) {
	    PORT_FreeArena(safe->poolp, PR_TRUE);
	}
    } else {
	if(safe != NULL) {
	    PORT_FreeArena(safe->poolp, PR_TRUE);
	}
    }

loser:
    if(nicks != NULL) {
	sec_pkcs12_destroy_nickname_list(nicks);
    }

    if(ref_certs != NULL) {
	sec_pkcs12_destroy_certificate_list(ref_certs);
    }

    if(pwitem != NULL) {
	SECITEM_ZfreeItem(pwitem, PR_TRUE);
    }

    if(problem == PR_TRUE) {
	dest = NULL;
    }

    return dest;
}
