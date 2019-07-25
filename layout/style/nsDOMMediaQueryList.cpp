






































#include "nsDOMMediaQueryList.h"
#include "nsPresContext.h"
#include "nsIMediaList.h"
#include "nsCSSParser.h"

nsDOMMediaQueryList::nsDOMMediaQueryList(nsPresContext *aPresContext,
                                         const nsAString &aMediaQueryList)
  : mPresContext(aPresContext),
    mMediaList(new nsMediaList),
    mMatchesValid(PR_FALSE)
{
  PR_INIT_CLIST(this);

  nsCSSParser parser;
  parser.ParseMediaList(aMediaQueryList, nsnull, 0, mMediaList, PR_FALSE);
}

nsDOMMediaQueryList::~nsDOMMediaQueryList()
{
  if (mPresContext) {
    PR_REMOVE_LINK(this);
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMMediaQueryList)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMMediaQueryList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPresContext)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSTARRAY_OF_NSCOMPTR(mListeners)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMMediaQueryList)
if (tmp->mPresContext) {
  PR_REMOVE_LINK(tmp);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPresContext)
}
NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mListeners)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

DOMCI_DATA(MediaQueryList, nsDOMMediaQueryList)

NS_INTERFACE_MAP_BEGIN(nsDOMMediaQueryList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaQueryList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMMediaQueryList)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MediaQueryList)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMMediaQueryList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMMediaQueryList)

NS_IMETHODIMP
nsDOMMediaQueryList::GetMedia(nsAString &aMedia)
{
  mMediaList->GetText(aMedia);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMediaQueryList::GetMatches(PRBool *aMatches)
{
  if (!mMatchesValid) {
    NS_ABORT_IF_FALSE(mListeners.Length() == 0,
                      "when listeners present, must keep mMatches current");
    RecomputeMatches();
  }

  *aMatches = mMatches;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMediaQueryList::AddListener(nsIDOMMediaQueryListListener *aListener)
{
  if (!mMatchesValid) {
    NS_ABORT_IF_FALSE(mListeners.Length() == 0,
                      "when listeners present, must keep mMatches current");
    RecomputeMatches();
  }

  if (!mListeners.Contains(aListener)) {
    mListeners.AppendElement(aListener);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMediaQueryList::RemoveListener(nsIDOMMediaQueryListListener *aListener)
{
  mListeners.RemoveElement(aListener);
  NS_ABORT_IF_FALSE(!mListeners.Contains(aListener),
                    "duplicate occurrence of listeners");
  return NS_OK;
}

void
nsDOMMediaQueryList::RecomputeMatches()
{
  if (!mPresContext) {
    return;
  }

  mMatches = mMediaList->Matches(mPresContext, nsnull);
  mMatchesValid = PR_TRUE;
}

void
nsDOMMediaQueryList::MediumFeaturesChanged(NotifyList &aListenersToNotify)
{
  mMatchesValid = PR_FALSE;

  if (mListeners.Length()) {
    PRPackedBool oldMatches = mMatches;
    RecomputeMatches();
    if (mMatches != oldMatches) {
      for (PRUint32 i = 0, i_end = mListeners.Length(); i != i_end; ++i) {
        HandleChangeData *d = aListenersToNotify.AppendElement();
        if (d) {
          d->mql = this;
          d->listener = mListeners[i];
        }
      }
    }
  }
}
