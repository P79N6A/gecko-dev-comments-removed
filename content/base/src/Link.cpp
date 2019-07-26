





#include "Link.h"

#include "mozilla/dom/Element.h"
#include "nsEventStates.h"
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

nsLinkState
Link::GetLinkState() const
{
  NS_ASSERTION(mRegistered,
               "Getting the link state of an unregistered Link!");
  return nsLinkState(mLinkState);
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

nsEventStates
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

  return nsEventStates();
}

already_AddRefed<nsIURI>
Link::GetURI() const
{
  nsCOMPtr<nsIURI> uri(mCachedURI);

  
  if (uri) {
    return uri.forget();
  }

  
  Link *self = const_cast<Link *>(this);
  Element *element = self->mElement;
  uri = element->GetHrefURI();

  
  if (uri) {
    mCachedURI = uri;
  }

  return uri.forget();
}

void
Link::SetProtocol(const nsAString &aProtocol)
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
Link::SetHost(const nsAString &aHost)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  
  
  

  
  nsAString::const_iterator start, end;
  aHost.BeginReading(start);
  aHost.EndReading(end);
  nsAString::const_iterator iter(start);
  (void)FindCharInReadable(':', iter, end);
  NS_ConvertUTF16toUTF8 host(Substring(start, iter));
  (void)uri->SetHost(host);

  
  if (iter != end) {
    iter++;
    if (iter != end) {
      nsAutoString portStr(Substring(iter, end));
      nsresult rv;
      int32_t port = portStr.ToInteger(&rv);
      if (NS_SUCCEEDED(rv)) {
        (void)uri->SetPort(port);
      }
    }
  };

  SetHrefAttribute(uri);
  return;
}

void
Link::SetHostname(const nsAString &aHostname)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  (void)uri->SetHost(NS_ConvertUTF16toUTF8(aHostname));
  SetHrefAttribute(uri);
}

void
Link::SetPathname(const nsAString &aPathname)
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
Link::SetSearch(const nsAString &aSearch)
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
Link::SetPort(const nsAString &aPort)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  nsresult rv;
  nsAutoString portStr(aPort);
  int32_t port = portStr.ToInteger(&rv);
  if (NS_FAILED(rv)) {
    return;
  }

  (void)uri->SetPort(port);
  SetHrefAttribute(uri);
}

void
Link::SetHash(const nsAString &aHash)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return;
  }

  (void)uri->SetRef(NS_ConvertUTF16toUTF8(aHash));
  SetHrefAttribute(uri);
}

void
Link::GetProtocol(nsAString &_protocol)
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
  _protocol.Append(PRUnichar(':'));
  return;
}

void
Link::GetHost(nsAString &_host)
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
Link::GetHostname(nsAString &_hostname)
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
Link::GetPathname(nsAString &_pathname)
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
Link::GetSearch(nsAString &_search)
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
Link::GetPort(nsAString &_port)
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
Link::GetHash(nsAString &_hash)
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
    _hash.Assign(PRUnichar('#'));
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

  
  mLinkState = defaultState;

  
  
  
  
  
  
  
  
  if (aNotify) {
    mElement->UpdateState(aNotify);
  } else {
    if (mLinkState == eLinkState_Unvisited) {
      mElement->UpdateLinkState(NS_EVENT_STATE_UNVISITED);
    } else {
      mElement->UpdateLinkState(nsEventStates());
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
Link::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
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

} 
} 
