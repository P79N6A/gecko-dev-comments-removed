





































#ifndef mozilla_places_SQLFunctions_h_
#define mozilla_places_SQLFunctions_h_






#include "mozIStorageFunction.h"

class mozIStorageConnection;

namespace mozilla {
namespace places {































class MatchAutoCompleteFunction : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);

private:
  


  static const PRUint32 kArgSearchString = 0;
  static const PRUint32 kArgIndexURL = 1;
  static const PRUint32 kArgIndexTitle = 2;
  static const PRUint32 kArgIndexTags = 3;
  static const PRUint32 kArgIndexVisitCount = 4;
  static const PRUint32 kArgIndexTyped = 5;
  static const PRUint32 kArgIndexBookmark = 6;
  static const PRUint32 kArgIndexMatchBehavior = 7;
  static const PRUint32 kArgIndexSearchBehavior = 8;
  static const PRUint32 kArgIndexLength = 9;

  


  typedef bool (*searchFunctionPtr)(const nsDependentSubstring &aToken,
                                    const nsAString &aSourceString);

  typedef nsAString::const_char_iterator const_wchar_iterator;

  







  static searchFunctionPtr getSearchFunction(PRInt32 aBehavior);

  









  static bool findAnywhere(const nsDependentSubstring &aToken,
                           const nsAString &aSourceString);

  








  static bool findBeginning(const nsDependentSubstring &aToken,
                            const nsAString &aSourceString);

  








  static bool findOnBoundary(const nsDependentSubstring &aToken,
                             const nsAString &aSourceString);

  








  static const_wchar_iterator nextWordBoundary(const_wchar_iterator aStart,
                                               const_wchar_iterator aEnd);
  











  static inline bool isWordBoundary(const PRUnichar &aChar);

  









  static void fixupURISpec(const nsCString &aURISpec, nsString &_fixedSpec);
};

} 
} 

#endif 
