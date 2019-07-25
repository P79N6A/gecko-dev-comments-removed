






































#include "Link.h"

#include "nsEventStates.h"
#include "nsIURL.h"

#include "nsContentUtils.h"
#include "nsEscape.h"
#include "nsGkAtoms.h"
#include "nsString.h"
#include "mozAutoDocUpdate.h"

#include "mozilla/Services.h"

namespace mozilla {
namespace dom {

Link::Link()
  : mLinkState(defaultState)
  , mRegistered(false)
  , mContent(NULL)
  , mHistory(services::GetHistoryService())
{
}

Link::~Link()
{
  UnregisterFromHistory();
}

nsLinkState
Link::GetLinkState() const
{
  NS_ASSERTION(mRegistered,
               "Getting the link state of an unregistered Link!");
  NS_ASSERTION(mLinkState != eLinkState_Unknown,
               "Getting the link state with an unknown value!");
  return mLinkState;
}

void
Link::SetLinkState(nsLinkState aState)
{
  NS_ASSERTION(mRegistered,
               "Setting the link state of an unregistered Link!");
  NS_ASSERTION(mLinkState != aState,
               "Setting state to the currently set state!");

  
  nsEventStates oldLinkState = LinkState();

  
  mLinkState = aState;

  
  mRegistered = false;

  
  nsIContent *content = Content();
  nsIDocument *doc = content->GetCurrentDoc();
  NS_ASSERTION(doc, "Registered but we have no document?!");
  nsEventStates newLinkState = LinkState();
  NS_ASSERTION(newLinkState == NS_EVENT_STATE_VISITED ||
               newLinkState == NS_EVENT_STATE_UNVISITED,
               "Unexpected state obtained from LinkState()!");
  mozAutoDocUpdate update(doc, UPDATE_CONTENT_STATE, PR_TRUE);
  doc->ContentStateChanged(content, oldLinkState ^ newLinkState);
}

nsEventStates
Link::LinkState() const
{
  
  
  Link *self = const_cast<Link *>(this);

  
  nsIContent *content = self->Content();
  if (!content->IsInDoc()) {
    self->mLinkState = eLinkState_Unvisited;
  }

  
  
  if (!mRegistered && mLinkState == eLinkState_Unknown) {
    
    nsCOMPtr<nsIURI> hrefURI(GetURI());
    if (!hrefURI) {
      self->mLinkState = eLinkState_NotLink;
      return nsEventStates();
    }

    
    nsresult rv = mHistory->RegisterVisitedCallback(hrefURI, self);
    if (NS_SUCCEEDED(rv)) {
      self->mRegistered = true;

      
      self->mLinkState = eLinkState_Unvisited;

      
      nsIDocument *doc = content->GetCurrentDoc();
      if (doc) {
        doc->AddStyleRelevantLink(self);
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
  nsIContent *content = self->Content();
  uri = content->GetHrefURI();

  
  if (uri && content->IsInDoc()) {
    mCachedURI = uri;
  }

  return uri.forget();
}

nsresult
Link::SetProtocol(const nsAString &aProtocol)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return NS_OK;
  }

  nsAString::const_iterator start, end;
  aProtocol.BeginReading(start);
  aProtocol.EndReading(end);
  nsAString::const_iterator iter(start);
  (void)FindCharInReadable(':', iter, end);
  (void)uri->SetScheme(NS_ConvertUTF16toUTF8(Substring(start, iter)));

  SetHrefAttribute(uri);
  return NS_OK;
}

nsresult
Link::SetHost(const nsAString &aHost)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return NS_OK;
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
      PRInt32 port = portStr.ToInteger((PRInt32 *)&rv);
      if (NS_SUCCEEDED(rv)) {
        (void)uri->SetPort(port);
      }
    }
  };

  SetHrefAttribute(uri);
  return NS_OK;
}

nsresult
Link::SetHostname(const nsAString &aHostname)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return NS_OK;
  }

  (void)uri->SetHost(NS_ConvertUTF16toUTF8(aHostname));
  SetHrefAttribute(uri);
  return NS_OK;
}

nsresult
Link::SetPathname(const nsAString &aPathname)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    return NS_OK;
  }

  (void)url->SetFilePath(NS_ConvertUTF16toUTF8(aPathname));
  SetHrefAttribute(uri);
  return NS_OK;
}

nsresult
Link::SetSearch(const nsAString &aSearch)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    return NS_OK;
  }

  (void)url->SetQuery(NS_ConvertUTF16toUTF8(aSearch));
  SetHrefAttribute(uri);
  return NS_OK;
}

nsresult
Link::SetPort(const nsAString &aPort)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return NS_OK;
  }

  nsresult rv;
  nsAutoString portStr(aPort);
  PRInt32 port = portStr.ToInteger((PRInt32 *)&rv);
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  (void)uri->SetPort(port);
  SetHrefAttribute(uri);
  return NS_OK;
}

nsresult
Link::SetHash(const nsAString &aHash)
{
  nsCOMPtr<nsIURI> uri(GetURIToMutate());
  if (!uri) {
    
    return NS_OK;
  }

  (void)uri->SetRef(NS_ConvertUTF16toUTF8(aHash));
  SetHrefAttribute(uri);
  return NS_OK;
}

nsresult
Link::GetProtocol(nsAString &_protocol)
{
  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    _protocol.AssignLiteral("http");
  }
  else {
    nsCAutoString scheme;
    (void)uri->GetScheme(scheme);
    CopyASCIItoUTF16(scheme, _protocol);
  }
  _protocol.Append(PRUnichar(':'));
  return NS_OK;
}

nsresult
Link::GetHost(nsAString &_host)
{
  _host.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    return NS_OK;
  }

  nsCAutoString hostport;
  nsresult rv = uri->GetHostPort(hostport);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(hostport, _host);
  }
  return NS_OK;
}

nsresult
Link::GetHostname(nsAString &_hostname)
{
  _hostname.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    return NS_OK;
  }

  nsCAutoString host;
  nsresult rv = uri->GetHost(host);
  
  
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(host, _hostname);
  }
  return NS_OK;
}

nsresult
Link::GetPathname(nsAString &_pathname)
{
  _pathname.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    
    return NS_OK;
  }

  nsCAutoString file;
  nsresult rv = url->GetFilePath(file);
  NS_ENSURE_SUCCESS(rv, rv);
  CopyUTF8toUTF16(file, _pathname);
  return NS_OK;
}

nsresult
Link::GetSearch(nsAString &_search)
{
  _search.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (!url) {
    
    
    return NS_OK;
  }

  nsCAutoString search;
  nsresult rv = url->GetQuery(search);
  if (NS_SUCCEEDED(rv) && !search.IsEmpty()) {
    CopyUTF8toUTF16(NS_LITERAL_CSTRING("?") + search, _search);
  }
  return NS_OK;
}

nsresult
Link::GetPort(nsAString &_port)
{
  _port.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    return NS_OK;
  }

  PRInt32 port;
  nsresult rv = uri->GetPort(&port);
  
  
  if (NS_SUCCEEDED(rv) && port != -1) {
    nsAutoString portStr;
    portStr.AppendInt(port, 10);
    _port.Assign(portStr);
  }
  return NS_OK;
}

nsresult
Link::GetHash(nsAString &_hash)
{
  _hash.Truncate();

  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    
    
    return NS_OK;
  }

  nsCAutoString ref;
  nsresult rv = uri->GetRef(ref);
  if (NS_SUCCEEDED(rv) && !ref.IsEmpty()) {
    NS_UnescapeURL(ref); 
    _hash.Assign(PRUnichar('#'));
    AppendUTF8toUTF16(ref, _hash);
  }
  return NS_OK;
}

void
Link::ResetLinkState(bool aNotify)
{
  
  if (mLinkState == defaultState) {
    return;
  }

  nsIContent *content = Content();

  
  nsIDocument *doc = content->GetCurrentDoc();
  if (doc && mLinkState != eLinkState_NotLink) {
    doc->ForgetLink(this);
  }

  UnregisterFromHistory();

  
  mLinkState = defaultState;

  
  mCachedURI = nsnull;

  
  
  
  
  
  if (aNotify && doc) {
    nsEventStates changedState = NS_EVENT_STATE_VISITED ^ NS_EVENT_STATE_UNVISITED;
    MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, aNotify);
    doc->ContentStateChanged(content, changedState);
  }
}

void
Link::UnregisterFromHistory()
{
  
  if (!mRegistered) {
    return;
  }

  NS_ASSERTION(mCachedURI, "mRegistered is true, but we have no cached URI?!");

  
  nsresult rv = mHistory->UnregisterVisitedCallback(mCachedURI, this);
  NS_ASSERTION(NS_SUCCEEDED(rv), "This should only fail if we misuse the API!");
  if (NS_SUCCEEDED(rv)) {
    mRegistered = false;
  }
}

already_AddRefed<nsIURI>
Link::GetURIToMutate()
{
  nsCOMPtr<nsIURI> uri(GetURI());
  if (!uri) {
    return nsnull;
  }
  nsCOMPtr<nsIURI> clone;
  (void)uri->Clone(getter_AddRefs(clone));
  return clone.forget();
}

void
Link::SetHrefAttribute(nsIURI *aURI)
{
  NS_ASSERTION(aURI, "Null URI is illegal!");

  nsCAutoString href;
  (void)aURI->GetSpec(href);
  (void)Content()->SetAttr(kNameSpaceID_None, nsGkAtoms::href,
                           NS_ConvertUTF8toUTF16(href), PR_TRUE);
}

nsIContent *
Link::Content()
{
  if (NS_LIKELY(mContent)) {
    return mContent;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(this));
  NS_ABORT_IF_FALSE(content, "This must be able to QI to nsIContent!");
  return mContent = content;
}

} 
} 
