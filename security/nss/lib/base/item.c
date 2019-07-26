



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: item.c,v $ $Revision: 1.5 $ $Date: 2012/04/25 14:49:26 $";
#endif 







#ifndef BASE_H
#include "base.h"
#endif 

















NSS_IMPLEMENT NSSItem *
nssItem_Create
(
  NSSArena *arenaOpt,
  NSSItem *rvOpt,
  PRUint32 length,
  const void *data
)
{
  NSSItem *rv = (NSSItem *)NULL;

#ifdef DEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSItem *)NULL;
    }
  }

  if( (const void *)NULL == data ) {
    if( length > 0 ) {
      nss_SetError(NSS_ERROR_INVALID_POINTER);
      return (NSSItem *)NULL;
    }
  }
#endif 

  if( (NSSItem *)NULL == rvOpt ) {
    rv = (NSSItem *)nss_ZNEW(arenaOpt, NSSItem);
    if( (NSSItem *)NULL == rv ) {
      goto loser;
    }
  } else {
    rv = rvOpt;
  }

  rv->size = length;
  rv->data = nss_ZAlloc(arenaOpt, length);
  if( (void *)NULL == rv->data ) {
    goto loser;
  }

  if( length > 0 ) {
    (void)nsslibc_memcpy(rv->data, data, length);
  }

  return rv;

 loser:
  if( rv != rvOpt ) {
    nss_ZFreeIf(rv);
  }

  return (NSSItem *)NULL;
}

NSS_IMPLEMENT void
nssItem_Destroy
(
  NSSItem *item
)
{
  nss_ClearErrorStack();

  nss_ZFreeIf(item->data);
  nss_ZFreeIf(item);

}

















NSS_IMPLEMENT NSSItem *
nssItem_Duplicate
(
  NSSItem *obj,
  NSSArena *arenaOpt,
  NSSItem *rvOpt
)
{
#ifdef DEBUG
  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSItem *)NULL;
    }
  }

  if( (NSSItem *)NULL == obj ) {
    nss_SetError(NSS_ERROR_INVALID_ITEM);
    return (NSSItem *)NULL;
  }
#endif 

  return nssItem_Create(arenaOpt, rvOpt, obj->size, obj->data);
}

#ifdef DEBUG













NSS_IMPLEMENT PRStatus
nssItem_verifyPointer
(
  const NSSItem *item
)
{
  if( ((const NSSItem *)NULL == item) ||
      (((void *)NULL == item->data) && (item->size > 0)) ) {
    nss_SetError(NSS_ERROR_INVALID_ITEM);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}
#endif 















NSS_IMPLEMENT PRBool
nssItem_Equal
(
  const NSSItem *one,
  const NSSItem *two,
  PRStatus *statusOpt
)
{
  if( (PRStatus *)NULL != statusOpt ) {
    *statusOpt = PR_SUCCESS;
  }

  if( ((const NSSItem *)NULL == one) && ((const NSSItem *)NULL == two) ) {
    return PR_TRUE;
  }

  if( ((const NSSItem *)NULL == one) || ((const NSSItem *)NULL == two) ) {
    return PR_FALSE;
  }

  if( one->size != two->size ) {
    return PR_FALSE;
  }

  return nsslibc_memequal(one->data, two->data, one->size, statusOpt);
}
