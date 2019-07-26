



#include "secplcy.h"
#include "prmem.h"

SECCipherFind *sec_CipherFindInit(PRBool onlyAllowed,
				  secCPStruct *policy,
				  long *ciphers)
{
  SECCipherFind *find = PR_NEWZAP(SECCipherFind);
  if (find)
    {
      find->policy = policy;
      find->ciphers = ciphers;
      find->onlyAllowed = onlyAllowed;
      find->index = -1;
    }
  return find;
}

long sec_CipherFindNext(SECCipherFind *find)
{
  char *policy;
  long rv = -1;
  secCPStruct *policies = (secCPStruct *) find->policy;
  long *ciphers = (long *) find->ciphers;
  long numCiphers = policies->num_ciphers;

  find->index++;
  while((find->index < numCiphers) && (rv == -1))
    {
      
      rv = ciphers[find->index];

      

      if (find->onlyAllowed)
	{
	  
	  policy = (&(policies->begin_ciphers)) + find->index + 1;

	  
	  if (! (*policy))
	    {
	      rv = -1;
	      find->index++;
	    }
	}
    }

  return rv;
}

char sec_IsCipherAllowed(long cipher, secCPStruct *policies,
			 long *ciphers)
{
  char result = SEC_CIPHER_NOT_ALLOWED; 
  long numCiphers = policies->num_ciphers;
  char *policy;
  int i;
  
  
  for (i=0, policy=(&(policies->begin_ciphers) + 1);
       i<numCiphers;
       i++, policy++)
    {
      if (cipher == ciphers[i])
	break;
    }

  if (i < numCiphers)
    {
      
      result = *policy;
    }

  return result;
}

void sec_CipherFindEnd(SECCipherFind *find)
{
  PR_FREEIF(find);
}
