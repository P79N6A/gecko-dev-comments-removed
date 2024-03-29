










#ifndef nsNavHistoryQuery_h_
#define nsNavHistoryQuery_h_






#include "mozilla/Attributes.h"

#define NS_NAVHISTORYQUERY_IID \
{ 0xb10185e0, 0x86eb, 0x4612, { 0x95, 0x7c, 0x09, 0x34, 0xf2, 0xb1, 0xce, 0xd7 } }

class nsNavHistoryQuery final : public nsINavHistoryQuery
{
public:
  nsNavHistoryQuery();
  

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERY_IID)
  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVHISTORYQUERY

  int32_t MinVisits() { return mMinVisits; }
  int32_t MaxVisits() { return mMaxVisits; }
  PRTime BeginTime() { return mBeginTime; }
  uint32_t BeginTimeReference() { return mBeginTimeReference; }
  PRTime EndTime() { return mEndTime; }
  uint32_t EndTimeReference() { return mEndTimeReference; }
  const nsString& SearchTerms() { return mSearchTerms; }
  bool OnlyBookmarked() { return mOnlyBookmarked; }
  bool DomainIsHost() { return mDomainIsHost; }
  const nsCString& Domain() { return mDomain; }
  bool UriIsPrefix() { return mUriIsPrefix; }
  nsIURI* Uri() { return mUri; } 
  bool AnnotationIsNot() { return mAnnotationIsNot; }
  const nsCString& Annotation() { return mAnnotation; }
  const nsTArray<int64_t>& Folders() const { return mFolders; }
  const nsTArray<nsString>& Tags() const { return mTags; }
  nsresult SetTags(const nsTArray<nsString>& aTags)
  {
    if (!mTags.ReplaceElementsAt(0, mTags.Length(), aTags))
      return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
  }
  bool TagsAreNot() { return mTagsAreNot; }

  const nsTArray<uint32_t>& Transitions() const { return mTransitions; }
  nsresult SetTransitions(const nsTArray<uint32_t>& aTransitions)
  {
    if (!mTransitions.ReplaceElementsAt(0, mTransitions.Length(),
                                        aTransitions))
      return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
  }

private:
  ~nsNavHistoryQuery() {}

protected:

  int32_t mMinVisits;
  int32_t mMaxVisits;
  PRTime mBeginTime;
  uint32_t mBeginTimeReference;
  PRTime mEndTime;
  uint32_t mEndTimeReference;
  nsString mSearchTerms;
  bool mOnlyBookmarked;
  bool mDomainIsHost;
  nsCString mDomain; 
  bool mUriIsPrefix;
  nsCOMPtr<nsIURI> mUri;
  bool mAnnotationIsNot;
  nsCString mAnnotation;
  nsTArray<int64_t> mFolders;
  nsTArray<nsString> mTags;
  bool mTagsAreNot;
  nsTArray<uint32_t> mTransitions;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryQuery, NS_NAVHISTORYQUERY_IID)



#define NS_NAVHISTORYQUERYOPTIONS_IID \
{0x95f8ba3b, 0xd681, 0x4d89, {0xab, 0xd1, 0xfd, 0xae, 0xf2, 0xa3, 0xde, 0x18}}

class nsNavHistoryQueryOptions final : public nsINavHistoryQueryOptions
{
public:
  nsNavHistoryQueryOptions()
  : mSort(0)
  , mResultType(0)
  , mExcludeItems(false)
  , mExcludeQueries(false)
  , mExcludeReadOnlyFolders(false)
  , mExpandQueries(true)
  , mIncludeHidden(false)
  , mMaxResults(0)
  , mQueryType(nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY)
  , mAsyncEnabled(false)
  { }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERYOPTIONS_IID)

  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVHISTORYQUERYOPTIONS

  uint16_t SortingMode() const { return mSort; }
  uint16_t ResultType() const { return mResultType; }
  bool ExcludeItems() const { return mExcludeItems; }
  bool ExcludeQueries() const { return mExcludeQueries; }
  bool ExcludeReadOnlyFolders() const { return mExcludeReadOnlyFolders; }
  bool ExpandQueries() const { return mExpandQueries; }
  bool IncludeHidden() const { return mIncludeHidden; }
  uint32_t MaxResults() const { return mMaxResults; }
  uint16_t QueryType() const { return mQueryType; }
  bool AsyncEnabled() const { return mAsyncEnabled; }

  nsresult Clone(nsNavHistoryQueryOptions **aResult);

private:
  ~nsNavHistoryQueryOptions() {}
  nsNavHistoryQueryOptions(const nsNavHistoryQueryOptions& other) {} 

  
  
  
  
  
  
  uint16_t mSort;
  nsCString mSortingAnnotation;
  nsCString mParentAnnotationToExclude;
  uint16_t mResultType;
  bool mExcludeItems;
  bool mExcludeQueries;
  bool mExcludeReadOnlyFolders;
  bool mExpandQueries;
  bool mIncludeHidden;
  uint32_t mMaxResults;
  uint16_t mQueryType;
  bool mAsyncEnabled;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryQueryOptions, NS_NAVHISTORYQUERYOPTIONS_IID)

#endif 
