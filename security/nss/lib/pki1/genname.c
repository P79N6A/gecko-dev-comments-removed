



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: genname.c,v $ $Revision: 1.3 $ $Date: 2005/01/20 02:25:49 $";
#endif 








#ifndef NSSBASE_H
#include "nssbase.h"
#endif 

#ifndef ASN1_H
#include "asn1.h"
#endif 

#ifndef PKI1_H
#include "pki1.h"
#endif 

























struct nssGeneralNameStr {
  NSSGeneralNameChoice choice;
  union {
    
    NSSUTF8 *rfc822Name;
    NSSUTF8 *dNSName;
    
    NSSName *directoryName;
    
    NSSUTF8 *uniformResourceIdentifier;
    NSSItem *iPAddress;
    NSSOID *registeredID;
  } u;
};
