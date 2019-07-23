



































#ifndef nsVerifier_h__
#define nsVerifier_h__

#include "nsPkgInt.h"

typedef enum {
   eStart = 0,
   eError = 1,
   eItsMe = 2 
} nsSMState;

typedef struct _nsVerifierMutable {
  const char* charset;
  nsPkgInt  cclass;
  PRUint32  stFactor; 
  nsPkgInt  states;
} nsVerifierMutable;

typedef const nsVerifierMutable nsVerifier;

#define GETCLASS(v,c) GETFROMPCK(((unsigned char)(c)), (v)->cclass)
#define GETNEXTSTATE(v,c,s) \
             GETFROMPCK((s)*((v)->stFactor)+GETCLASS((v),(c)), ((v)->states))

#endif 
