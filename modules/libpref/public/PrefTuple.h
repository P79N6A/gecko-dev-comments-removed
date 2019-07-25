



































#ifndef preftuple_included
#define preftuple_included

#include "nsTArray.h"
#include "nsString.h"

struct PrefTuple
{
  nsCAutoString key;

  
  
  nsCAutoString stringVal;
  PRInt32       intVal;
  bool          boolVal;

  enum {
    PREF_STRING,
    PREF_INT,
    PREF_BOOL
  } type;

};

#endif

