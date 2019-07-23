









































#include "mozStorageUnicodeFunctions.h"
#include "nsUnicharUtils.h"

int
StorageUnicodeFunctions::RegisterFunctions(sqlite3 *aDB)
{
  struct Functions {
    const char *zName;
    int nArg;
    int enc;
    void *pContext;
    void (*xFunc)(sqlite3_context*, int, sqlite3_value**);
  } functions[] = {
    {"lower", 1, SQLITE_UTF16, 0,        caseFunction},
    {"lower", 1, SQLITE_UTF8,  0,        caseFunction},
    {"upper", 1, SQLITE_UTF16, (void*)1, caseFunction},
    {"upper", 1, SQLITE_UTF8,  (void*)1, caseFunction},

    {"like",  2, SQLITE_UTF16, 0,        likeFunction},
    {"like",  2, SQLITE_UTF8,  0,        likeFunction},
    {"like",  3, SQLITE_UTF16, 0,        likeFunction},
    {"like",  3, SQLITE_UTF8,  0,        likeFunction},
  };

  int rv = SQLITE_OK;
  for (unsigned i = 0; SQLITE_OK == rv && i < NS_ARRAY_LENGTH(functions); ++i) {
    struct Functions *p = &functions[i];
    rv = sqlite3_create_function(aDB, p->zName, p->nArg, p->enc, p->pContext,
                                 p->xFunc, NULL, NULL);
  }

  return rv;
}

void
StorageUnicodeFunctions::caseFunction(sqlite3_context *p,
                                      int aArgc,
                                      sqlite3_value **aArgv)
{
  NS_ASSERTION(1 == aArgc, "Invalid number of arguments!");

  nsAutoString data(static_cast<const PRUnichar *>(sqlite3_value_text16(aArgv[0])));
  PRBool toUpper = sqlite3_user_data(p) ? PR_TRUE : PR_FALSE;

  if (toUpper)
    ToUpperCase(data);
  else 
    ToLowerCase(data);

  
  sqlite3_result_text16(p, nsPromiseFlatString(data).get(), -1,
                        SQLITE_TRANSIENT);
}

static int
likeCompare(nsAString::const_iterator aPatternItr,
            nsAString::const_iterator aPatternEnd,
            nsAString::const_iterator aStringItr,
            nsAString::const_iterator aStringEnd,
            const nsAString::char_type *aEscape)
{
  const PRUnichar MATCH_ALL('%');
  const PRUnichar MATCH_ONE('_');

  PRBool lastWasEscape = PR_FALSE;
  while (aPatternItr != aPatternEnd) {
    







    if (!lastWasEscape && *aPatternItr == MATCH_ALL) {
      
      




      while (*aPatternItr == MATCH_ALL || *aPatternItr == MATCH_ONE) {
        if (*aPatternItr == MATCH_ONE) {
          
          if (aStringItr == aStringEnd)
            return 0;
          aStringItr++;
        }
        aPatternItr++;
      }

      
      if (aPatternItr == aPatternEnd)
        return 1;

      while (aStringItr != aStringEnd) {
        if (likeCompare(aPatternItr, aPatternEnd, aStringItr, aStringEnd, aEscape)) {
          
          return 1;
        }
        aStringItr++;
      }

      
      return 0;
    } else if (!lastWasEscape && *aPatternItr == MATCH_ONE) {
      
      if (aStringItr == aStringEnd) {
        
        return 0;
      }
      aStringItr++;
      lastWasEscape = PR_FALSE;
    } else if (!lastWasEscape && *aPatternItr == *aEscape) {
      
      lastWasEscape = PR_TRUE;
    } else {
      
      if (ToUpperCase(*aStringItr) != ToUpperCase(*aPatternItr)) {
        
        return 0;
      }
      aStringItr++;
      lastWasEscape = PR_FALSE;
    }
    
    aPatternItr++;
  }

  return aStringItr == aStringEnd;
}






void
StorageUnicodeFunctions::likeFunction(sqlite3_context *p,
                                      int aArgc,
                                      sqlite3_value **aArgv)
{
  NS_ASSERTION(2 == aArgc || 3 == aArgc, "Invalid number of arguments!");

  if (sqlite3_value_bytes(aArgv[0]) > SQLITE_MAX_LIKE_PATTERN_LENGTH) {
    sqlite3_result_error(p, "LIKE or GLOB pattern too complex", SQLITE_TOOBIG);
    return;
  }

  nsAutoString A(static_cast<const PRUnichar *>(sqlite3_value_text16(aArgv[1])));
  nsAutoString B(static_cast<const PRUnichar *>(sqlite3_value_text16(aArgv[0])));
  NS_ASSERTION(B.Length() != 0, "LIKE string must not be null!");

  nsAutoString E;
  if (3 == aArgc) {
    E = static_cast<const PRUnichar *>(sqlite3_value_text16(aArgv[2]));
    NS_ASSERTION(E.Length() == 1, "ESCAPE express must be a single character");
  }

  nsAString::const_iterator itrString, endString;
  A.BeginReading(itrString);
  A.EndReading(endString);
  nsAString::const_iterator itrPattern, endPattern;
  B.BeginReading(itrPattern);
  B.EndReading(endPattern);
  sqlite3_result_int(p, likeCompare(itrPattern, endPattern,
                                    itrString, endString, E.get()));
}

