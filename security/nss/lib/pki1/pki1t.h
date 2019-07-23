



































#ifndef PKI1T_H
#define PKI1T_H

#ifdef DEBUG
static const char PKI1T_CVS_ID[] = "@(#) $RCSfile: pki1t.h,v $ $Revision: 1.3 $ $Date: 2005/01/20 02:25:49 $";
#endif 








#ifndef BASET_H
#include "baset.h"
#endif 

#ifndef NSSPKI1T_H
#include "nsspki1t.h"
#endif 

PR_BEGIN_EXTERN_C














struct NSSOIDStr {
#ifdef DEBUG
  const NSSUTF8 *tag;
  const NSSUTF8 *expl;
#endif 
  NSSItem data;
};


















struct nssAttributeTypeAliasTableStr {
  const NSSUTF8 *alias;
  const NSSOID **oid;
};
typedef struct nssAttributeTypeAliasTableStr nssAttributeTypeAliasTable;

PR_END_EXTERN_C

#endif 
