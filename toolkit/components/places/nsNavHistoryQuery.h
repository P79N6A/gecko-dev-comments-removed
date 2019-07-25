











































#ifndef nsNavHistoryQuery_h_
#define nsNavHistoryQuery_h_






#define NS_NAVHISTORYQUERY_IID \
{ 0xb10185e0, 0x86eb, 0x4612, { 0x95, 0x7c, 0x09, 0x34, 0xf2, 0xb1, 0xce, 0xd7 } }

class nsNavHistoryQuery : public nsINavHistoryQuery
{
public:
  nsNavHistoryQuery();
  

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERY_IID)
  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVHISTORYQUERY

  PRInt32 MinVisits() { return mMinVisits; }
  PRInt32 MaxVisits() { return mMaxVisits; }
  PRTime BeginTime() { return mBeginTime; }
  PRUint32 BeginTimeReference() { return mBeginTimeReference; }
  PRTime EndTime() { return mEndTime; }
  PRUint32 EndTimeReference() { return mEndTimeReference; }
  const nsString& SearchTerms() { return mSearchTerms; }
  bool OnlyBookmarked() { return mOnlyBookmarked; }
  bool DomainIsHost() { return mDomainIsHost; }
  const nsCString& Domain() { return mDomain; }
  bool UriIsPrefix() { return mUriIsPrefix; }
  nsIURI* Uri() { return mUri; } 
  bool AnnotationIsNot() { return mAnnotationIsNot; }
  const nsCString& Annotation() { return mAnnotation; }
  const nsTArray<PRInt64>& Folders() const { return mFolders; }
  const nsTArray<nsString>& Tags() const { return mTags; }
  nsresult SetTags(const nsTArray<nsString>& aTags)
  {
    if (!mTags.ReplaceElementsAt(0, mTags.Length(), aTags))
      return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
  }
  bool TagsAreNot() { return mTagsAreNot; }

  const nsTArray<PRUint32>& Transitions() const { return mTransitions; }
  nsresult SetTransitions(const nsTArray<PRUint32>& aTransitions)
  {
    if (!mTransitions.ReplaceElementsAt(0, mTransitions.Length(),
                                        aTransitions))
      return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
  }

private:
  ~nsNavHistoryQuery() {}

protected:

  PRInt32 mMinVisits;
  PRInt32 mMaxVisits;
  PRTime mBeginTime;
  PRUint32 mBeginTimeReference;
  PRTime mEndTime;
  PRUint32 mEndTimeReference;
  nsString mSearchTerms;
  bool mOnlyBookmarked;
  bool mDomainIsHost;
  nsCString mDomain; 
  bool mUriIsPrefix;
  nsCOMPtr<nsIURI> mUri;
  bool mAnnotationIsNot;
  nsCString mAnnotation;
  nsTArray<PRInt64> mFolders;
  nsTArray<nsString> mTags;
  bool mTagsAreNot;
  nsTArray<PRUint32> mTransitions;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryQuery, NS_NAVHISTORYQUERY_IID)



#define NS_NAVHISTORYQUERYOPTIONS_IID \
{0x95f8ba3b, 0xd681, 0x4d89, {0xab, 0xd1, 0xfd, 0xae, 0xf2, 0xa3, 0xde, 0x18}}

class nsNavHistoryQueryOptions : public nsINavHistoryQueryOptions
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
  , mRedirectsMode(nsINavHistoryQueryOptions::REDIRECTS_MODE_ALL)
  , mMaxResults(0)
  , mQueryType(nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY)
  , mAsyncEnabled(false)
  { }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERYOPTIONS_IID)

  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVHISTORYQUERYOPTIONS

  PRUint16 SortingMode() const { return mSort; }
  PRUint16 ResultType() const { return mResultType; }
  bool ExcludeItems() const { return mExcludeItems; }
  bool ExcludeQueries() const { return mExcludeQueries; }
  bool ExcludeReadOnlyFolders() const { return mExcludeReadOnlyFolders; }
  bool ExpandQueries() const { return mExpandQueries; }
  bool IncludeHidden() const { return mIncludeHidden; }
  PRUint16 RedirectsMode() const { return mRedirectsMode; }
  PRUint32 MaxResults() const { return mMaxResults; }
  PRUint16 QueryType() const { return mQueryType; }
  bool AsyncEnabled() const { return mAsyncEnabled; }

  nsresult Clone(nsNavHistoryQueryOptions **aResult);

private:
  nsNavHistoryQueryOptions(const nsNavHistoryQueryOptions& other) {} 

  
  
  
  
  
  
  PRUint16 mSort;
  nsCString mSortingAnnotation;
  nsCString mParentAnnotationToExclude;
  PRUint16 mResultType;
  bool mExcludeItems;
  bool mExcludeQueries;
  bool mExcludeReadOnlyFolders;
  bool mExpandQueries;
  bool mIncludeHidden;
  PRUint16 mRedirectsMode;
  PRUint32 mMaxResults;
  PRUint16 mQueryType;
  bool mAsyncEnabled;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryQueryOptions, NS_NAVHISTORYQUERYOPTIONS_IID)

#endif 
