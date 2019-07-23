











































#ifndef nsNavHistoryQuery_h_
#define nsNavHistoryQuery_h_






#define NS_NAVHISTORYQUERY_IID \
{ 0xb10185e0, 0x86eb, 0x4612, { 0x95, 0x7c, 0x09, 0x34, 0xf2, 0xb1, 0xce, 0xd7 } }

class nsNavHistoryQuery : public nsINavHistoryQuery
{
public:
  nsNavHistoryQuery();
  

#ifdef MOZILLA_1_8_BRANCH
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERY_IID)
#else
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERY_IID)
#endif
  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVHISTORYQUERY

  PRInt32 MinVisits() { return mMinVisits; }
  PRInt32 MaxVisits() { return mMaxVisits; }
  PRTime BeginTime() { return mBeginTime; }
  PRUint32 BeginTimeReference() { return mBeginTimeReference; }
  PRTime EndTime() { return mEndTime; }
  PRUint32 EndTimeReference() { return mEndTimeReference; }
  const nsString& SearchTerms() { return mSearchTerms; }
  PRBool OnlyBookmarked() { return mOnlyBookmarked; }
  PRBool DomainIsHost() { return mDomainIsHost; }
  const nsCString& Domain() { return mDomain; }
  PRBool UriIsPrefix() { return mUriIsPrefix; }
  nsIURI* Uri() { return mUri; } 
  PRBool AnnotationIsNot() { return mAnnotationIsNot; }
  const nsCString& Annotation() { return mAnnotation; }
  const nsTArray<PRInt64>& Folders() const { return mFolders; }

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
  PRBool mOnlyBookmarked;
  PRBool mDomainIsHost;
  nsCString mDomain; 
  PRBool mUriIsPrefix;
  nsCOMPtr<nsIURI> mUri;
  PRBool mAnnotationIsNot;
  nsCString mAnnotation;
  nsTArray<PRInt64> mFolders;
};

#ifndef MOZILLA_1_8_BRANCH
NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryQuery, NS_NAVHISTORYQUERY_IID)
#endif



#define NS_NAVHISTORYQUERYOPTIONS_IID \
{0x95f8ba3b, 0xd681, 0x4d89, {0xab, 0xd1, 0xfd, 0xae, 0xf2, 0xa3, 0xde, 0x18}}

class nsNavHistoryQueryOptions : public nsINavHistoryQueryOptions
{
public:
  nsNavHistoryQueryOptions() : mSort(0), mResultType(0),
                               mGroupCount(0), mGroupings(nsnull),
                               mExcludeItems(PR_FALSE),
                               mExcludeQueries(PR_FALSE),
                               mExcludeReadOnlyFolders(PR_FALSE),
                               mExpandQueries(PR_FALSE),
                               mIncludeHidden(PR_FALSE),
                               mShowSessions(PR_FALSE),
                               mMaxResults(0),
                               mQueryType(nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY)
  { }

#ifdef MOZILLA_1_8_BRANCH
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERYOPTIONS_IID)
#else
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NAVHISTORYQUERYOPTIONS_IID)
#endif

  NS_DECL_ISUPPORTS
  NS_DECL_NSINAVHISTORYQUERYOPTIONS

  PRUint16 SortingMode() const { return mSort; }
  PRUint16 ResultType() const { return mResultType; }
  const PRUint16* GroupingMode(PRUint32 *count) const {
    *count = mGroupCount; return mGroupings;
  }
  PRBool ExcludeItems() const { return mExcludeItems; }
  PRBool ExcludeQueries() const { return mExcludeQueries; }
  PRBool ExcludeReadOnlyFolders() const { return mExcludeReadOnlyFolders; }
  PRBool ExpandQueries() const { return mExpandQueries; }
  PRBool IncludeHidden() const { return mIncludeHidden; }
  PRBool ShowSessions() const { return mShowSessions; }
  PRUint32 MaxResults() const { return mMaxResults; }
  PRUint16 QueryType() const { return mQueryType; }

  nsresult Clone(nsNavHistoryQueryOptions **aResult);

private:
  nsNavHistoryQueryOptions(const nsNavHistoryQueryOptions& other) {} 

  ~nsNavHistoryQueryOptions() { delete[] mGroupings; }

  
  
  
  
  
  
  PRUint16 mSort;
  nsCString mSortingAnnotation;

  PRUint16 mResultType;
  PRUint32 mGroupCount;
  PRUint16 *mGroupings;
  PRPackedBool mExcludeItems;
  PRPackedBool mExcludeQueries;
  PRPackedBool mExcludeReadOnlyFolders;
  PRPackedBool mExpandQueries;
  PRPackedBool mIncludeHidden;
  PRPackedBool mShowSessions;
  PRUint32 mMaxResults;
  PRUint16 mQueryType;
};

#ifndef MOZILLA_1_8_BRANCH
NS_DEFINE_STATIC_IID_ACCESSOR(nsNavHistoryQueryOptions, NS_NAVHISTORYQUERYOPTIONS_IID)
#endif

#endif 

