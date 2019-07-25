




#ifndef mozilla_places_SQLFunctions_h_
#define mozilla_places_SQLFunctions_h_






#include "mozIStorageFunction.h"
#include "mozilla/Attributes.h"

class mozIStorageConnection;

namespace mozilla {
namespace places {



































class MatchAutoCompleteFunction MOZ_FINAL : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);

private:
  


  static const uint32_t kArgSearchString = 0;
  static const uint32_t kArgIndexURL = 1;
  static const uint32_t kArgIndexTitle = 2;
  static const uint32_t kArgIndexTags = 3;
  static const uint32_t kArgIndexVisitCount = 4;
  static const uint32_t kArgIndexTyped = 5;
  static const uint32_t kArgIndexBookmark = 6;
  static const uint32_t kArgIndexOpenPageCount = 7;
  static const uint32_t kArgIndexMatchBehavior = 8;
  static const uint32_t kArgIndexSearchBehavior = 9;
  static const uint32_t kArgIndexLength = 10;

  


  typedef bool (*searchFunctionPtr)(const nsDependentCSubstring &aToken,
                                    const nsACString &aSourceString);

  typedef nsACString::const_char_iterator const_char_iterator;

  







  static searchFunctionPtr getSearchFunction(int32_t aBehavior);

  








  static bool findBeginning(const nsDependentCSubstring &aToken,
                            const nsACString &aSourceString);

  








  static bool findBeginningCaseSensitive(const nsDependentCSubstring &aToken,
                                         const nsACString &aSourceString);

  









  static bool findAnywhere(const nsDependentCSubstring &aToken,
                           const nsACString &aSourceString);

  








  static bool findOnBoundary(const nsDependentCSubstring &aToken,
                             const nsACString &aSourceString);


  












  static void fixupURISpec(const nsCString &aURISpec, int32_t aMatchBehavior,
                           nsCString &_fixedSpec);
};























class CalculateFrecencyFunction MOZ_FINAL : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);
};







class GenerateGUIDFunction MOZ_FINAL : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);
};









class GetUnreversedHostFunction MOZ_FINAL : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);
};














class FixupURLFunction MOZ_FINAL : public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION

  





  static nsresult create(mozIStorageConnection *aDBConn);
};

} 
} 

#endif 
