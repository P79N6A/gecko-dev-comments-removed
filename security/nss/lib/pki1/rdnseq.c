



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: rdnseq.c,v $ $Revision: 1.3 $ $Date: 2005/01/20 02:25:49 $";
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












struct nssRDNSeqStr {
  PRUint32 seqSize;
  NSSRDN **rdns;
};
