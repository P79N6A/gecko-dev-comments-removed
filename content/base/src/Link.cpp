





#include "Link.h"

#include "mozilla/EventStates.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Element.h"
#include "nsIURL.h"
#include "nsISizeOf.h"

#include "nsEscape.h"
#include "nsGkAtoms.h"
#include "nsString.h"
#include "mozAutoDocUpdate.h"

#include "mozilla/Services.h"

namespace mozilla {
namespace dom {

Link::Link(Element *aElement)
  : mElement(aElement)
  , mHistory(services::GetHistoryService())
  , mLinkState(eLinkState_NotLink)
  , mNeedsRegistration(false)
  , mRegistered(false)
{
  NS_ABORT_IF_FALSE(mElement, "Must have an element");
}

Link::~Link()
{
  UnregisterFromHistory();
}

bool
Link::ElementHasHref() const
{
  return ((!mElement->IsSVG() && mElement->HasAttr(kNameSpaceID_None, nsGkAtoms::href))
        || (!mElement->IsHTML() && mElement->HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)));
}

void
Link::SetLinkState(nsLinkState aState)
{
  NS_ASSERTION(mRegistered,
               "Setting the link state of an unregistered Link!");
  NS_ASSERTION(mLinkState != aState,
               "Setting state to the currently set state!");

  
  mLinkState = aState;

  
  mRegistered = false;

  NS_ABORT_IF_FALSE(LinkState() == NS_EVENT_STATE_VISITED ||
                    LinkState() == NS_EVENT_STATE_UNVISITED,
                    "Unexpected state obtained from LinkState()!");

  
  mElement->UpdateState(true);
}

EventStates
Link::LinkState() const
{
  
  
  Link *self = const_cast<Link *>(this);

  Element *element = self->mElement;

  
  
  if (!mRegistered && mNeedsRegistration && element->IsInDoc()) {
    
    self->mNeedsRegistration = false;

    nsCOMPtr<nsIURI> hrefURI(GetURI());

    
    self->mLinkState = eLinkState_Unvisited;

    
    
    if (mHistory && hrefURI) {
      nsresult rv = mHistory->RegisterVisitedCallback(hrefURI, self);
      if (NS_SUCCEEDED(rv)) {
        self->mRegistered = true;

        
        element->GetCurrentDoc()->AddStyleRelevantLink(self);
      }
    }
  }

  
  if (mLinkState == eLinkState_Visited) {
    return NS_EVENT_STATE_VISITED;
  }

  if (mLinkState == eLinkState_Unvisited) {
    return NS_EVENT_STATE_UNVISITED;
  }

  return EventStates();
}

nsIURI*
Link::GetURI() const
{
  
  if (mCachedURI) {
    return mCachedURI;
  }

  
  Link *self = const_cast<Link *>(this);
  Element *element = self->mElement;
  mCachedURI = element->GetHrefURI();

  return mCachedURI;
}

void
Link::SetProtocol(const nsAString &aProtocol, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  nsAString::const_iterator start, end;
  aProtocol.BeginReading(start);
  aProtocol.EndReading(end);
  nsAString::const_iterator iter(start);
  (void)FindCharInReadable(':', iter, end);
  (void)uri->SetScheme(NS_ConvertUTF16toUTF8(Substring(start, iter)));

  SetHrefAttribute(uri);
}

void
Link::SetPassword(const nsAString &aPassword, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  uri->SetPassword(NS_ConvertUTF16toUTF8(aPassword));
  SetHrefAttribute(uri);
}

void
Link::SetUsername(const nsAString &aUsername, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  uri->SetUsername(NS_ConvertUTF16toUTF8(aUsername));
  SetHrefAttribute(uri);
}

void
Link::SetHost(const nsAString &aHost, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  (void)uri->SetHostPort(NS_ConvertUTF16toUTF8(aHost));
  SetHrefAttribute(uri);
}

void
Link::SetHostname(const nsAString &aHostname, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  (void)uri->SetHost(NS_ConvertUTF16toUTF8(aHostname));
  SetHrefAttribute(uri);
}

void
Link::SetPathname(const nsAString &aPathname, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    return;
  }

  (void)url->SetFilePath(NS_ConvertUTF16toUTF8(aPathname));
  SetHrefAttribute(uri);
}

void
Link::SetSearch(const nsAString& aSearch, ErrorResult& aError)
{
  SetSearchInternal(aSearch);
  UpdateURLSearchParams();
}

void
Link::SetSearchInternal(const nsAString& aSearch)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    return;
  }

  (void)url->SetQuery(NS_ConvertUTF16toUTF8(aSearch));
  SetHrefAttribute(uri);
}

void
Link::SetPort(const nsAString &aPort, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  nsresult rv;
  nsAutoString portStr(aPort);

  
  int32_t port = -1;
  if (!aPort.IsEmpty()) {
    port = portStr.ToInteger(&rv);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  (void)uri->SetPort(port);
  SetHrefAttribute(uri);
}

void
Link::SetHash(const nsAString &aHash, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  (void)uri->SetRef(NS_ConvertUTF16toUTF8(aHash));
  SetHrefAttribute(uri);
}

void
Link::GetOrigin(nsAString &aOrigin, ErrorResult& aError)
{
  aOrigin.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    return;
  }

  nsString origin;
  nsContentUtils::GetUTFOrigin(uri, origin);
  aOrigin.Assign(origin);
}

void
Link::GetProtocol(nsAString &_protocol, ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    _protocol.AssignLiteral("http");
  }
  else {
    nsAutoCString scheme;
    (void)uri->GetScheme(scheme);
    CopyASCIItoUTF16(scheme, _protocol);
  }
  _protocol.Append(char16_t(':'));
  return;
}

void
Link::GetUsername(nsAString& aUsername, ErrorResult& aError)
{
  aUsername.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    return;
  }

  nsAutoCString username;
  uri->GetUsername(username);
  CopyASCIItoUTF16(username, aUsername);
}

void
Link::GetPassword(nsAString &aPassword, ErrorResult& aError)
{
  aPassword.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    return;
  }

  nsAutoCString password;
  uri->GetPassword(password);
  CopyASCIItoUTF16(password, aPassword);
}

void
Link::GetHost(nsAString &_host, ErrorResult& aError)
{
  _host.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    return;
  }

  nsAutoCString hostport;
  nsresult rv = uri->GetHostPort(hostport);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(hostport, _host);
  }
}

void
Link::GetHostname(nsAString &_hostname, ErrorResult& aError)
{
  _hostname.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    return;
  }

  nsAutoCString host;
  nsresult rv = uri->GetHost(host);
  
  
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(host, _hostname);
  }
}

void
Link::GetPathname(nsAString &_pathname, ErrorResult& aError)
{
  _pathname.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    
    return;
  }

  nsAutoCString file;
  nsresult rv = url->GetFilePath(file);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(file, _pathname);
  }
}

void
Link::GetSearch(nsAString &_search, ErrorResult& aError)
{
  _search.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    
    return;
  }

  nsAutoCString search;
  nsresult rv = url->GetQuery(search);
  if (NS_SUCCEEDED(rv) && !search.IsEmpty()) {
    CopyUTF8toUTF16(NS_LITERAL_CSTRING("?") + search, _search);
  }
}

void
Link::GetPort(nsAString &_port, ErrorResult& aError)
{
  _port.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    return;
  }

  int32_t port;
  nsresult rv = uri->GetPort(&port);
  
  
  if (NS_SUCCEEDED(rv) && port != -1) {
    nsAutoString portStr;
    portStr.AppendInt(port, 10);
    _port.Assign(portStr);
  }
}

void
Link::GetHash(nsAString &_hash, ErrorResult& aError)
{
  _hash.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    
    return;
  }

  nsAutoCString ref;
  nsresult rv = uri->GetRef(ref);
  if (NS_SUCCEEDED(rv) && !ref.IsEmpty()) {
    NS_UnescapeURL(ref); 
    _hash.Assign(char16_t('#'));
    AppendUTF8toUTF16(ref, _hash);
  }
}

void
Link::ResetLinkState(bool aNotify, bool aHasHref)
{
  nsLinkState defaultState;

  
  if (aHasHref) {
    defaultState = eLinkState_Unvisited;
  } else {
    defaultState = eLinkState_NotLink;
  }

  
  
  
  if (!mNeedsRegistration && mLinkState != eLinkState_NotLink) {
    nsIDocument *doc = mElement->GetCurrentDoc();
    if (doc && (mRegistered || mLinkState == eLinkState_Visited)) {
      
      
      doc->ForgetLink(this);
    }

    UnregisterFromHistory();
  }

  
  mNeedsRegistration = aHasHref;

  
  mCachedURI = nullptr;
  UpdateURLSearchParams();

  
  mLinkState = defaultState;

  
  
  
  
  
  
  
  
  if (aNotify) {
    mElement->UpdateState(aNotify);
  } else {
    if (mLinkState == eLinkState_Unvisited) {
      mElement->UpdateLinkState(NS_EVENT_STATE_UNVISITED);
    } else {
      mElement->UpdateLinkState(EventStates());
    }
  }
}

void
Link::UnregisterFromHistory()
{
  
  if (!mRegistered) {
    return;
  }

  NS_ASSERTION(mCachedURI, "mRegistered is true, but we have no cached URI?!");

  
  if (mHistory) {
    nsresult rv = mHistory->UnregisterVisitedCallback(mCachedURI, this);
    NS_ASSERTION(NS_SUCCEEDED(rv), "This should only fail if we misuse the API!");
    if (NS_SUCCEEDED(rv)) {
      mRegistered = false;
    }
  }
}

already_AddRefed<nsIURI>
Link::GetURIToMutate()
{
  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    return nullptr;
  }
  nsCOMPtr<nsIURI> clone;
  (void)uri->Clone(getter_AddRefs(clone));
  return clone.forget();
}

void
Link::SetHrefAttribute(nsIURI *aURI)
{
  NS_ASSERTION(aURI, "Null URI is illegal!");

  
  
  
  nsAutoCString href;
  (void)aURI->GetSpec(href);
  (void)mElement->SetAttr(kNameSpaceID_None, nsGkAtoms::href,
                          NS_ConvertUTF8toUTF16(href), true);
}

size_t
Link::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;

  if (mCachedURI) {
    nsCOMPtr<nsISizeOf> iface = do_QueryInterface(mCachedURI);
    if (iface) {
      n += iface->SizeOfIncludingThis(aMallocSizeOf);
    }
  }

  
  
  

  return n;
}

URLSearchParams*
Link::SearchParams()
{
  CreateSearchParamsIfNeeded();
  return mSearchParams;
}

void
Link::SetSearchParams(URLSearchParams& aSearchParams)
{
  if (mSearchParams) {
    mSearchParams->RemoveObserver(this);
  }

  mSearchParams = &aSearchParams;
  mSearchParams->AddObserver(this);

  nsAutoString search;
  mSearchParams->Serialize(search);
  SetSearchInternal(search);
}

void
Link::URLSearchParamsUpdated(URLSearchParams* aSearchParams)
{
  MOZ_ASSERT(mSearchParams);
  MOZ_ASSERT(mSearchParams == aSearchParams);

  nsString search;
  mSearchParams->Serialize(search);
  SetSearchInternal(search);
}

void
Link::UpdateURLSearchParams()
{
  if (!mSearchParams) {
    return;
  }

  nsAutoCString search;
  nsCOMPtr<nsIURI> uri(GetURI());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (url) {
    nsresult rv = url->GetQuery(search);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to get the query from a nsIURL.");
    }
  }

  mSearchParams->ParseInput(search, this);
}

void
Link::CreateSearchParamsIfNeeded()
{
  if (!mSearchParams) {
    mSearchParams = new URLSearchParams();
    mSearchParams->AddObserver(this);
    UpdateURLSearchParams();
  }
}

void
Link::Unlink()
{
  if (mSearchParams) {
    mSearchParams->RemoveObserver(this);
    mSearchParams = nullptr;
  }
}

void
Link::Traverse(nsCycleCollectionTraversalCallback &cb)
{
  Link* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSearchParams);
}

} 
} 
