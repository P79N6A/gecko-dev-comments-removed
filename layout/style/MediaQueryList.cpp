






#include "mozilla/dom/MediaQueryList.h"
#include "nsPresContext.h"
#include "nsIMediaList.h"
#include "nsCSSParser.h"
#include "nsIDocument.h"

namespace mozilla {
namespace dom {

MediaQueryList::MediaQueryList(nsPresContext *aPresContext,
                               const nsAString &aMediaQueryList)
  : mPresContext(aPresContext),
    mMediaList(new nsMediaList),
    mMatchesValid(false)
{
  PR_INIT_CLIST(this);

  SetIsDOMBinding();

  nsCSSParser parser;
  parser.ParseMediaList(aMediaQueryList, nullptr, 0, mMediaList, false);
}

MediaQueryList::~MediaQueryList()
{
  if (mPresContext) {
    PR_REMOVE_LINK(this);
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(MediaQueryList)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(MediaQueryList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPresContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCallbacks)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(MediaQueryList)
  if (tmp->mPresContext) {
    PR_REMOVE_LINK(tmp);
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mPresContext)
  }
  tmp->RemoveAllListeners();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(MediaQueryList)

NS_INTERFACE_MAP_BEGIN(MediaQueryList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(MediaQueryList)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(MediaQueryList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MediaQueryList)

void
MediaQueryList::GetMedia(nsAString &aMedia)
{
  mMediaList->GetText(aMedia);
}

bool
MediaQueryList::Matches()
{
  if (!mMatchesValid) {
    NS_ABORT_IF_FALSE(!HasListeners(),
                      "when listeners present, must keep mMatches current");
    RecomputeMatches();
  }

  return mMatches;
}

void
MediaQueryList::AddListener(MediaQueryListListener& aListener)
{
  if (!HasListeners()) {
    
    
    
    NS_ADDREF_THIS();
  }

  if (!mMatchesValid) {
    NS_ABORT_IF_FALSE(!HasListeners(),
                      "when listeners present, must keep mMatches current");
    RecomputeMatches();
  }

  for (uint32_t i = 0; i < mCallbacks.Length(); ++i) {
    if (aListener == *mCallbacks[i]) {
      
      return;
    }
  }

  mCallbacks.AppendElement(&aListener);
  if (!HasListeners()) {
    
    NS_RELEASE_THIS();
  }
}

void
MediaQueryList::RemoveListener(MediaQueryListListener& aListener)
{
  for (uint32_t i = 0; i < mCallbacks.Length(); ++i) {
    if (aListener == *mCallbacks[i]) {
      mCallbacks.RemoveElementAt(i);
      if (!HasListeners()) {
        
        NS_RELEASE_THIS();
      }
      break;
    }
  }
}

void
MediaQueryList::RemoveAllListeners()
{
  bool hadListeners = HasListeners();
  mCallbacks.Clear();
  if (hadListeners) {
    
    NS_RELEASE_THIS();
  }
}

void
MediaQueryList::RecomputeMatches()
{
  if (!mPresContext) {
    return;
  }

  mMatches = mMediaList->Matches(mPresContext, nullptr);
  mMatchesValid = true;
}

void
MediaQueryList::MediumFeaturesChanged(NotifyList &aListenersToNotify)
{
  mMatchesValid = false;

  if (HasListeners()) {
    bool oldMatches = mMatches;
    RecomputeMatches();
    if (mMatches != oldMatches) {
      for (uint32_t i = 0, i_end = mCallbacks.Length(); i != i_end; ++i) {
        HandleChangeData *d = aListenersToNotify.AppendElement();
        if (d) {
          d->mql = this;
          d->callback = mCallbacks[i];
        }
      }
    }
  }
}

nsISupports*
MediaQueryList::GetParentObject() const
{
  if (!mPresContext) {
    return nullptr;
  }
  return mPresContext->Document();
}

JSObject*
MediaQueryList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MediaQueryListBinding::Wrap(aCx, aScope, this);
}

} 
} 
