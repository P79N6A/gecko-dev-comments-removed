






#ifndef _SOFTKVER_H_
#define _SOFTKVER_H_

#ifndef NSS_DISABLE_ECC
#ifdef NSS_ECC_MORE_THAN_SUITE_B
#define SOFTOKEN_ECC_STRING " Extended ECC"
#else
#define SOFTOKEN_ECC_STRING " Basic ECC"
#endif
#else
#define SOFTOKEN_ECC_STRING ""
#endif








#define SOFTOKEN_VERSION  "3.19" SOFTOKEN_ECC_STRING " Beta"
#define SOFTOKEN_VMAJOR   3
#define SOFTOKEN_VMINOR   19
#define SOFTOKEN_VPATCH   0
#define SOFTOKEN_VBUILD   0
#define SOFTOKEN_BETA     PR_TRUE

#endif 
