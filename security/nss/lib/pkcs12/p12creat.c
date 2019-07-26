



#include "pkcs12.h"
#include "secitem.h"
#include "secport.h"
#include "secder.h"
#include "secoid.h"
#include "p12local.h"
#include "secerr.h"






SEC_PKCS12PFXItem *
sec_pkcs12_new_pfx(void)
{
    SEC_PKCS12PFXItem   *pfx = NULL;
    PLArenaPool     *poolp = NULL;

    poolp = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);	
    if(poolp == NULL)
	goto loser;

    pfx = (SEC_PKCS12PFXItem *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12PFXItem));
    if(pfx == NULL)
	goto loser;
    pfx->poolp = poolp;

    return pfx;

loser:
    PORT_FreeArena(poolp, PR_TRUE);
    return NULL;
}





SEC_PKCS12AuthenticatedSafe *
sec_pkcs12_new_asafe(PLArenaPool *poolp)
{
    SEC_PKCS12AuthenticatedSafe  *asafe = NULL;
    void *mark;

    mark = PORT_ArenaMark(poolp);
    asafe = (SEC_PKCS12AuthenticatedSafe *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12AuthenticatedSafe));
    if(asafe == NULL)
	goto loser;
    asafe->poolp = poolp;
    PORT_Memset(&asafe->old_baggage, 0, sizeof(SEC_PKCS12Baggage_OLD));

    PORT_ArenaUnmark(poolp, mark);
    return asafe;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}




SEC_PKCS12SafeContents *
sec_pkcs12_create_safe_contents(PLArenaPool *poolp)
{
    SEC_PKCS12SafeContents *safe;
    void *mark;

    if(poolp == NULL)
	return NULL;

    
    mark = PORT_ArenaMark(poolp);
    safe = (SEC_PKCS12SafeContents *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12SafeContents));
    if(safe == NULL)
    {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }

    
    safe->contents = (SEC_PKCS12SafeBag**)PORT_ArenaZAlloc(poolp, 
						  sizeof(SEC_PKCS12SafeBag *));
    if(safe->contents == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }
    safe->contents[0] = NULL;
    safe->poolp       = poolp;
    safe->safe_size   = 0;
    PORT_ArenaUnmark(poolp, mark);
    return safe;
}





SEC_PKCS12BaggageItem *
sec_pkcs12_create_external_bag(SEC_PKCS12Baggage *luggage)
{
    void *dummy, *mark;
    SEC_PKCS12BaggageItem *bag;

    if(luggage == NULL) {
	return NULL;
    }

    mark = PORT_ArenaMark(luggage->poolp);

    
    if(luggage->bags == NULL) {
	luggage->bags=(SEC_PKCS12BaggageItem**)PORT_ArenaZAlloc(luggage->poolp, 
					sizeof(SEC_PKCS12BaggageItem *));
	if(luggage->bags == NULL) {
	    goto loser;
	}
	luggage->luggage_size = 0;
    }

        
    dummy = PORT_ArenaGrow(luggage->poolp, luggage->bags,
    			sizeof(SEC_PKCS12BaggageItem *) * (luggage->luggage_size + 1),
    			sizeof(SEC_PKCS12BaggageItem *) * (luggage->luggage_size + 2));
    if(dummy == NULL) {
	goto loser;
    }
    luggage->bags = (SEC_PKCS12BaggageItem**)dummy;

    luggage->bags[luggage->luggage_size] = 
    		(SEC_PKCS12BaggageItem *)PORT_ArenaZAlloc(luggage->poolp,
    							sizeof(SEC_PKCS12BaggageItem));
    if(luggage->bags[luggage->luggage_size] == NULL) {
	goto loser;
    }

    
    bag = luggage->bags[luggage->luggage_size];
    bag->espvks = (SEC_PKCS12ESPVKItem **)PORT_ArenaZAlloc(
    						luggage->poolp,
    						sizeof(SEC_PKCS12ESPVKItem *));
    bag->unencSecrets = (SEC_PKCS12SafeBag **)PORT_ArenaZAlloc(
    						luggage->poolp,
    						sizeof(SEC_PKCS12SafeBag *));
    if((bag->espvks == NULL) || (bag->unencSecrets == NULL)) {
	goto loser;
    }

    bag->poolp = luggage->poolp;
    luggage->luggage_size++;
    luggage->bags[luggage->luggage_size] = NULL;
    bag->espvks[0] = NULL;
    bag->unencSecrets[0] = NULL;
    bag->nEspvks = bag->nSecrets = 0;

    PORT_ArenaUnmark(luggage->poolp, mark);
    return bag;

loser:
    PORT_ArenaRelease(luggage->poolp, mark);
    PORT_SetError(SEC_ERROR_NO_MEMORY);
    return NULL;
}


SEC_PKCS12Baggage *
sec_pkcs12_create_baggage(PLArenaPool *poolp)
{
    SEC_PKCS12Baggage *luggage;
    void *mark;

    if(poolp == NULL)
	return NULL;

    mark = PORT_ArenaMark(poolp);

    
    luggage = (SEC_PKCS12Baggage *)PORT_ArenaZAlloc(poolp, 
	sizeof(SEC_PKCS12Baggage));
    if(luggage == NULL)
    {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }

    
    luggage->bags = (SEC_PKCS12BaggageItem **)PORT_ArenaZAlloc(poolp,
    					sizeof(SEC_PKCS12BaggageItem *));
    if(luggage->bags == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }

    luggage->bags[0] = NULL;
    luggage->luggage_size = 0;
    luggage->poolp = poolp;

    PORT_ArenaUnmark(poolp, mark);
    return luggage;
}


void 
SEC_PKCS12DestroyPFX(SEC_PKCS12PFXItem *pfx)
{
    if (pfx != NULL && pfx->poolp != NULL)
    {
	PORT_FreeArena(pfx->poolp, PR_TRUE);
    }
}
