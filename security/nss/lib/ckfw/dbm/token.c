



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: token.c,v $ $Revision: 1.5 $ $Date: 2012/04/25 14:49:30 $";
#endif 

#include "ckdbm.h"

static CK_RV
nss_dbm_mdToken_Setup
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  nss_dbm_token_t *token = (nss_dbm_token_t *)mdToken->etc;
  CK_RV rv = CKR_OK;

  token->arena = NSSCKFWToken_GetArena(fwToken, &rv);
  token->session_db = nss_dbm_db_open(token->arena, fwInstance, (char *)NULL, 
                                      O_RDWR|O_CREAT, &rv);
  if( (nss_dbm_db_t *)NULL == token->session_db ) {
    return rv;
  }

  

  return CKR_OK;
}

static void
nss_dbm_mdToken_Invalidate
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  nss_dbm_token_t *token = (nss_dbm_token_t *)mdToken->etc;

  if( (nss_dbm_db_t *)NULL != token->session_db ) {
    nss_dbm_db_close(token->session_db);
    token->session_db = (nss_dbm_db_t *)NULL;
  }
}

static CK_RV
nss_dbm_mdToken_InitToken
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSItem *pin,
  NSSUTF8 *label
)
{
  nss_dbm_token_t *token = (nss_dbm_token_t *)mdToken->etc;
  nss_dbm_instance_t *instance = (nss_dbm_instance_t *)mdInstance->etc;
  CK_RV rv;

  
  
  if( (nss_dbm_db_t *)NULL != token->session_db ) {
    nss_dbm_db_close(token->session_db);
  }

  token->session_db = nss_dbm_db_open(token->arena, fwInstance, (char *)NULL, 
                                      O_RDWR|O_CREAT, &rv);
  if( (nss_dbm_db_t *)NULL == token->session_db ) {
    return rv;
  }

  

  if( token->slot->flags & O_RDWR ) {
    if( (nss_dbm_db_t *)NULL != token->slot->token_db ) {
      nss_dbm_db_close(token->slot->token_db);
    }

    token->slot->token_db = nss_dbm_db_open(instance->arena, fwInstance, 
                                            token->slot->filename,
                                            token->slot->flags | O_CREAT | O_TRUNC, 
                                            &rv);
    if( (nss_dbm_db_t *)NULL == token->slot->token_db ) {
      return rv;
    }

    

    rv = nss_dbm_db_set_label(token->slot->token_db, label);
    if( CKR_OK != rv ) {
      return rv;
    }
  }

  return CKR_OK;
}

static NSSUTF8 *
nss_dbm_mdToken_GetLabel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  nss_dbm_token_t *token = (nss_dbm_token_t *)mdToken->etc;

  if( (NSSUTF8 *)NULL == token->label ) {
    token->label = nss_dbm_db_get_label(token->slot->token_db, token->arena, pError);
  }

  
  if( (NSSUTF8 *)NULL == token->label ) {
    return token->slot->filename;
  }

  return token->label;
}

static NSSUTF8 *
nss_dbm_mdToken_GetManufacturerID
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return "mozilla.org NSS";
}

static NSSUTF8 *
nss_dbm_mdToken_GetModel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return "dbm";
}




static CK_BBOOL
nss_dbm_mdToken_GetIsWriteProtected
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  nss_dbm_token_t *token = (nss_dbm_token_t *)mdToken->etc;

  if( token->slot->flags & O_RDWR ) {
    return CK_FALSE;
  } else {
    return CK_TRUE;
  }
}








static CK_ULONG
nss_dbm_mdToken_effectively_infinite
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return CK_EFFECTIVELY_INFINITE;
}

static CK_VERSION
nss_dbm_mdToken_GetHardwareVersion
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  nss_dbm_token_t *token = (nss_dbm_token_t *)mdToken->etc;
  return nss_dbm_db_get_format_version(token->slot->token_db);
}




static NSSCKMDSession *
nss_dbm_mdToken_OpenSession
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKFWSession *fwSession,
  CK_BBOOL rw,
  CK_RV *pError
)
{
  nss_dbm_token_t *token = (nss_dbm_token_t *)mdToken->etc;
  return nss_dbm_mdSession_factory(token, fwSession, fwInstance, rw, pError);
}





NSS_IMPLEMENT NSSCKMDToken *
nss_dbm_mdToken_factory
(
  nss_dbm_slot_t *slot,
  CK_RV *pError
)
{
  nss_dbm_token_t *token;
  NSSCKMDToken *rv;

  token = nss_ZNEW(slot->instance->arena, nss_dbm_token_t);
  if( (nss_dbm_token_t *)NULL == token ) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKMDToken *)NULL;
  }

  rv = nss_ZNEW(slot->instance->arena, NSSCKMDToken);
  if( (NSSCKMDToken *)NULL == rv ) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKMDToken *)NULL;
  }

  token->slot = slot;

  rv->etc = (void *)token;
  rv->Setup = nss_dbm_mdToken_Setup;
  rv->Invalidate = nss_dbm_mdToken_Invalidate;
  rv->InitToken = nss_dbm_mdToken_InitToken;
  rv->GetLabel = nss_dbm_mdToken_GetLabel;
  rv->GetManufacturerID = nss_dbm_mdToken_GetManufacturerID;
  rv->GetModel = nss_dbm_mdToken_GetModel;
  
  
  rv->GetIsWriteProtected = nss_dbm_mdToken_GetIsWriteProtected;
  
  
  
  
  
  
  rv->GetMaxSessionCount = nss_dbm_mdToken_effectively_infinite;
  rv->GetMaxRwSessionCount = nss_dbm_mdToken_effectively_infinite;
  
  
  
  
  
  
  rv->GetHardwareVersion = nss_dbm_mdToken_GetHardwareVersion;
  
  
  rv->OpenSession = nss_dbm_mdToken_OpenSession;
  rv->null = NULL;

  return rv;
}
