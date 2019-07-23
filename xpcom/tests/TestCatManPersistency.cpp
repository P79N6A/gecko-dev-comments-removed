




































#include "TestHarness.h"
#include "nsICategoryManager.h"
#include "nsXPCOMCID.h"

#define CMPT_CAT         "CatManPersistencyTestCategory"
#define CMPT_ENTRY       "CatManPersistencyTestEntry"
#define CMPT_VALUE_TRUE  "CatManPersistencyTestValueTrue"
#define CMPT_VALUE_FALSE "CatManPersistencyTestValueFalse"

int main(int argc, char** argv)
{
  
  
  
  
  
  

  if (argc < 2)
  {
    fail("no phase parameter");
    return NS_ERROR_FAILURE;
  }

  char phasename[] = "TestCatManPersistency Phase X";
  phasename[strlen(phasename) - 1] = argv[1][0];

  
  ScopedXPCOM xpcom(phasename);
  if (xpcom.failed())
  {
    fail("no ScopedXPCOM");
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
  {
    fail("no catman");
    return rv;
  }

  
  nsCString previousValue;
  switch (argv[1][0])
  {
    case '1':
      
      
      
      rv = catman->AddCategoryEntry(CMPT_CAT,
                                    CMPT_ENTRY,
                                    CMPT_VALUE_FALSE,
                                    PR_TRUE,  
                                    PR_TRUE,  
                                    nsnull);
      if (NS_FAILED(rv))
      {
        fail("AddCategoryEntry(false) failed");
        return rv;
      }
      break;

    case '2':
      
      
      
      
      rv = catman->AddCategoryEntry(CMPT_CAT,
                                    CMPT_ENTRY,
                                    CMPT_VALUE_TRUE,
                                    PR_TRUE,  
                                    PR_TRUE,  
                                    getter_Copies(previousValue));
      if (NS_FAILED(rv))
      {
        fail("AddCategoryEntry(true) failed");
        return rv;
      }
      if (strcmp(previousValue.get(), CMPT_VALUE_FALSE))
      {
        fail("initialization failed");
        return rv;
      }
      break;

    case '3':
      
      
      
      rv = catman->GetCategoryEntry(CMPT_CAT,
                                    CMPT_ENTRY,
                                    getter_Copies(previousValue));
      if (NS_FAILED(rv))
      {
        fail("GetCategoryEntry failed");
        return rv;
      }
      if (strcmp(previousValue.get(), CMPT_VALUE_TRUE))
      {
        
        fail("setting value failed");
        return rv;
      }
      break;

    case '4':
      
      
      
      rv = catman->DeleteCategory(CMPT_CAT);
      if (NS_FAILED(rv))
      {
        fail("DeleteCategory failed");
        return rv;
      }
      break;

    case '5':
      
      
      
      rv = catman->GetCategoryEntry(CMPT_CAT,
                                    CMPT_ENTRY,
                                    getter_Copies(previousValue));
      if (NS_SUCCEEDED(rv))
      {
        fail("category wasn't deleted");
        return rv;
      }
      break;

    default:
      fail("invalid phase parameter '%s'", argv[1]);
      return NS_ERROR_FAILURE;
  }


  
  
  
  passed(phasename);
  return NS_OK; 
}
