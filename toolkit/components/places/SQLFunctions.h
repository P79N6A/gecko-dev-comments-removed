






































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
  static const PRUint32 kArgIndexOpenPageCount = 7;
  static const PRUint32 kArgIndexMatchBehavior = 8;
  static const PRUint32 kArgIndexSearchBehavior = 9;
  static const PRUint32 kArgIndexLength = 10;

  


  typedef bool (*searchFunctionPtr)(const nsDependentCSubstring &aToken,
                                    const nsACString &aSourceString);

  typedef nsACString::const_char_iterator const_char_iterator;

  







  static searchFunctionPtr getSearchFunction(PRInt32 aBehavior);

  








  static bool findBeginning(const nsDependentCSubstring &aToken,
                            const nsACString &aSourceString);

  









  static bool findAnywhere(const nsDependentCSubstring &aToken,
                           const nsACString &aSourceString);

  








  static bool findOnBoundary(const nsDependentCSubstring &aToken,
                             const nsACString &aSourceString);


  












  static void fixupURISpec(const nsCString &aURISpec, PRInt32 aMatchBehavior,
                           nsCString &_fixedSpec);
};























class CalculateFrecencyFunction : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);
};







class GenerateGUIDFunction : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);
};









class GetUnreversedHostFunction : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);
};

} 
} 

#endif 
