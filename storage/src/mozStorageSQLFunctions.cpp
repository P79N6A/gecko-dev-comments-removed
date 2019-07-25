









































#include "mozilla/Util.h"

#include "mozStorageSQLFunctions.h"
#include "nsUnicharUtils.h"

namespace mozilla {
namespace storage {




namespace {

















int
likeCompare(nsAString::const_iterator aPatternItr,
            nsAString::const_iterator aPatternEnd,
            nsAString::const_iterator aStringItr,
            nsAString::const_iterator aStringEnd,
            PRUnichar aEscapeChar)
{
  const PRUnichar MATCH_ALL('%');
  const PRUnichar MATCH_ONE('_');

  bool lastWasEscape = false;
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
        if (likeCompare(aPatternItr, aPatternEnd, aStringItr, aStringEnd,
                        aEscapeChar)) {
          
          return 1;
        }
        aStringItr++;
      }

      
      return 0;
    }
    else if (!lastWasEscape && *aPatternItr == MATCH_ONE) {
      
      if (aStringItr == aStringEnd) {
        
        return 0;
      }
      aStringItr++;
      lastWasEscape = false;
    }
    else if (!lastWasEscape && *aPatternItr == aEscapeChar) {
      
      lastWasEscape = true;
    }
    else {
      
      if (::ToUpperCase(*aStringItr) != ::ToUpperCase(*aPatternItr)) {
        
        return 0;
      }
      aStringItr++;
      lastWasEscape = false;
    }

    aPatternItr++;
  }

  return aStringItr == aStringEnd;
}











template <class T, size_t N> class AutoArray
{

public:

  AutoArray(size_t size)
  : mBuffer(size <= N ? mAutoBuffer : new T[size])
  {
  }

  ~AutoArray()
  { 
    if (mBuffer != mAutoBuffer)
      delete[] mBuffer; 
  }

  





  T *get() 
  {
    return mBuffer; 
  }

private:
  T *mBuffer;           
  T mAutoBuffer[N];     
};












int
levenshteinDistance(const nsAString &aStringS,
                    const nsAString &aStringT,
                    int *_result)
{
    
    *_result = -1;

    const PRUint32 sLen = aStringS.Length();
    const PRUint32 tLen = aStringT.Length();

    if (sLen == 0) {
      *_result = tLen;
      return SQLITE_OK;
    }
    if (tLen == 0) {
      *_result = sLen;
      return SQLITE_OK;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    AutoArray<int, nsAutoString::kDefaultStorageSize> row1(sLen + 1);
    AutoArray<int, nsAutoString::kDefaultStorageSize> row2(sLen + 1);

    
    int *prevRow = row1.get();
    NS_ENSURE_TRUE(prevRow, SQLITE_NOMEM);
    int *currRow = row2.get();
    NS_ENSURE_TRUE(currRow, SQLITE_NOMEM);

    
    for (PRUint32 i = 0; i <= sLen; i++)
        prevRow[i] = i;

    const PRUnichar *s = aStringS.BeginReading();
    const PRUnichar *t = aStringT.BeginReading();

    
    
    for (PRUint32 ti = 1; ti <= tLen; ti++) {

        
        currRow[0] = ti;

        
        const PRUnichar tch = t[ti - 1];

        
        
        for (PRUint32 si = 1; si <= sLen; si++) {
            
            
            
            const PRUnichar sch = s[si - 1];
            int cost = (sch == tch) ? 0 : 1;

            
            
            
            
            int aPrime = prevRow[si - 1] + cost;
            int bPrime = prevRow[si] + 1;
            int cPrime = currRow[si - 1] + 1;
            currRow[si] = NS_MIN(aPrime, NS_MIN(bPrime, cPrime));
        }

        
        
        
        
        int *oldPrevRow = prevRow;
        prevRow = currRow;
        currRow = oldPrevRow;
    }

    
    
    *_result = prevRow[sLen];
    return SQLITE_OK;
}



struct Functions {
  const char *zName;
  int nArg;
  int enc;
  void *pContext;
  void (*xFunc)(::sqlite3_context*, int, sqlite3_value**);
};

} 




int
registerFunctions(sqlite3 *aDB)
{
  Functions functions[] = {
    {"lower",               
      1, 
      SQLITE_UTF16, 
      0,        
      caseFunction},
    {"lower",               
      1, 
      SQLITE_UTF8,  
      0,        
      caseFunction},
    {"upper",               
      1, 
      SQLITE_UTF16, 
      (void*)1, 
      caseFunction},
    {"upper",               
      1, 
      SQLITE_UTF8,  
      (void*)1, 
      caseFunction},

    {"like",                
      2, 
      SQLITE_UTF16, 
      0,        
      likeFunction},
    {"like",                
      2, 
      SQLITE_UTF8,  
      0,        
      likeFunction},
    {"like",                
      3, 
      SQLITE_UTF16, 
      0,        
      likeFunction},
    {"like",                
      3, 
      SQLITE_UTF8,  
      0,        
      likeFunction},

    {"levenshteinDistance", 
      2, 
      SQLITE_UTF16, 
      0,        
      levenshteinDistanceFunction},
    {"levenshteinDistance", 
      2, 
      SQLITE_UTF8,  
      0,        
      levenshteinDistanceFunction},
  };

  int rv = SQLITE_OK;
  for (size_t i = 0; SQLITE_OK == rv && i < ArrayLength(functions); ++i) {
    struct Functions *p = &functions[i];
    rv = ::sqlite3_create_function(aDB, p->zName, p->nArg, p->enc, p->pContext,
                                   p->xFunc, NULL, NULL);
  }

  return rv;
}




void
caseFunction(sqlite3_context *aCtx,
             int aArgc,
             sqlite3_value **aArgv)
{
  NS_ASSERTION(1 == aArgc, "Invalid number of arguments!");

  nsAutoString data(static_cast<const PRUnichar *>(::sqlite3_value_text16(aArgv[0])));
  bool toUpper = ::sqlite3_user_data(aCtx) ? true : false;

  if (toUpper)
    ::ToUpperCase(data);
  else
    ::ToLowerCase(data);

  
  ::sqlite3_result_text16(aCtx, data.get(), -1, SQLITE_TRANSIENT);
}






void
likeFunction(sqlite3_context *aCtx,
             int aArgc,
             sqlite3_value **aArgv)
{
  NS_ASSERTION(2 == aArgc || 3 == aArgc, "Invalid number of arguments!");

  if (::sqlite3_value_bytes(aArgv[0]) > SQLITE_MAX_LIKE_PATTERN_LENGTH) {
    ::sqlite3_result_error(aCtx, "LIKE or GLOB pattern too complex",
                           SQLITE_TOOBIG);
    return;
  }

  if (!::sqlite3_value_text16(aArgv[0]) || !::sqlite3_value_text16(aArgv[1]))
    return;

  nsDependentString A(static_cast<const PRUnichar *>(::sqlite3_value_text16(aArgv[1])));
  nsDependentString B(static_cast<const PRUnichar *>(::sqlite3_value_text16(aArgv[0])));
  NS_ASSERTION(!B.IsEmpty(), "LIKE string must not be null!");

  PRUnichar E = 0;
  if (3 == aArgc)
    E = static_cast<const PRUnichar *>(::sqlite3_value_text16(aArgv[2]))[0];

  nsAString::const_iterator itrString, endString;
  A.BeginReading(itrString);
  A.EndReading(endString);
  nsAString::const_iterator itrPattern, endPattern;
  B.BeginReading(itrPattern);
  B.EndReading(endPattern);
  ::sqlite3_result_int(aCtx, likeCompare(itrPattern, endPattern, itrString,
                                         endString, E));
}

void levenshteinDistanceFunction(sqlite3_context *aCtx,
                                 int aArgc,
                                 sqlite3_value **aArgv)
{
  NS_ASSERTION(2 == aArgc, "Invalid number of arguments!");

  
  if (::sqlite3_value_type(aArgv[0]) == SQLITE_NULL ||
      ::sqlite3_value_type(aArgv[1]) == SQLITE_NULL) {
    ::sqlite3_result_null(aCtx);
    return;
  }

  int aLen = ::sqlite3_value_bytes16(aArgv[0]) / sizeof(PRUnichar);
  const PRUnichar *a = static_cast<const PRUnichar *>(::sqlite3_value_text16(aArgv[0]));

  int bLen = ::sqlite3_value_bytes16(aArgv[1]) / sizeof(PRUnichar);
  const PRUnichar *b = static_cast<const PRUnichar *>(::sqlite3_value_text16(aArgv[1]));

  
  int distance = -1;
  const nsDependentString A(a, aLen);
  const nsDependentString B(b, bLen);
  int status = levenshteinDistance(A, B, &distance);
  if (status == SQLITE_OK) {
    ::sqlite3_result_int(aCtx, distance);    
  }
  else if (status == SQLITE_NOMEM) {
    ::sqlite3_result_error_nomem(aCtx);
  }
  else {
    ::sqlite3_result_error(aCtx, "User function returned error code", -1);
  }
}

} 
} 
