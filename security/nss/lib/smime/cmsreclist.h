







#ifndef _CMSRECLIST_H
#define _CMSRECLIST_H

struct NSSCMSRecipientStr {
    int				riIndex;	
    int				subIndex;	
						
    enum {RLIssuerSN=0, RLSubjKeyID=1} kind;	
    union {
	CERTIssuerAndSN *	issuerAndSN;
	SECItem *		subjectKeyID;
    } id;

    
    CERTCertificate *		cert;
    SECKEYPrivateKey *		privkey;
    PK11SlotInfo *		slot;
};

typedef struct NSSCMSRecipientStr NSSCMSRecipient;

#endif 
