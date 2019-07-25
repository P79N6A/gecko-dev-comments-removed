



#include "nsIEHistoryEnumerator.h"

#include <urlhist.h>
#include <shlguid.h>

#include "nsStringAPI.h"
#include "nsNetUtil.h"
#include "nsIVariant.h"
#include "nsCOMArray.h"
#include "nsArrayEnumerator.h"

namespace {

  PRTime FileTimeToPRTime(FILETIME* filetime)
  {
    SYSTEMTIME st;
    ::FileTimeToSystemTime(filetime, &st);
    PRExplodedTime prt;
    prt.tm_year = st.wYear;
    
    
    prt.tm_month = st.wMonth - 1;
    prt.tm_mday = st.wDay;
    prt.tm_hour = st.wHour;
    prt.tm_min = st.wMinute;
    prt.tm_sec = st.wSecond;
    prt.tm_usec = st.wMilliseconds * 1000;
    prt.tm_wday = 0;
    prt.tm_yday = 0;
    prt.tm_params.tp_gmt_offset = 0;
    prt.tm_params.tp_dst_offset = 0;
    return PR_ImplodeTime(&prt);
  }

} 




NS_IMPL_ISUPPORTS1(nsIEHistoryEnumerator, nsISimpleEnumerator)

nsIEHistoryEnumerator::nsIEHistoryEnumerator()
{
  ::CoInitialize(NULL);  
}

nsIEHistoryEnumerator::~nsIEHistoryEnumerator()
{
  ::CoUninitialize();
}

void
nsIEHistoryEnumerator::EnsureInitialized()
{
  if (mURLEnumerator)
    return;

  HRESULT hr = ::CoCreateInstance(CLSID_CUrlHistory,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IUrlHistoryStg2,
                                  getter_AddRefs(mIEHistory));
  if (FAILED(hr))
    return;

  hr = mIEHistory->EnumUrls(getter_AddRefs(mURLEnumerator));
  if (FAILED(hr))
    return;
}

NS_IMETHODIMP
nsIEHistoryEnumerator::HasMoreElements(bool* _retval)
{
  *_retval = false;

  EnsureInitialized();
  MOZ_ASSERT(mURLEnumerator, "Should have instanced an IE History URLEnumerator");
  if (!mURLEnumerator)
    return NS_OK;

  STATURL statURL;
  ULONG fetched;

  
  HRESULT hr = mURLEnumerator->Next(1, &statURL, &fetched);
  if (FAILED(hr) || fetched != 1UL) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> uri;
  if (statURL.pwcsUrl) {
    nsDependentString url(statURL.pwcsUrl);
    nsresult rv = NS_NewURI(getter_AddRefs(uri), url);
    ::CoTaskMemFree(statURL.pwcsUrl);
    if (NS_FAILED(rv)) {
      
      return HasMoreElements(_retval);
    }
  }

  nsDependentString title(statURL.pwcsTitle);

  PRTime lastVisited = FileTimeToPRTime(&(statURL.ftLastVisited));

  mCachedNextEntry = do_CreateInstance("@mozilla.org/hash-property-bag;1");
  MOZ_ASSERT(mCachedNextEntry, "Should have instanced a new property bag");
  if (mCachedNextEntry) {
    mCachedNextEntry->SetPropertyAsInterface(NS_LITERAL_STRING("uri"), uri);
    mCachedNextEntry->SetPropertyAsAString(NS_LITERAL_STRING("title"), title);
    mCachedNextEntry->SetPropertyAsInt64(NS_LITERAL_STRING("time"), lastVisited);

    *_retval = true;
  }

  if (statURL.pwcsTitle)
    ::CoTaskMemFree(statURL.pwcsTitle);

  return NS_OK;
}

NS_IMETHODIMP
nsIEHistoryEnumerator::GetNext(nsISupports** _retval)
{
  *_retval = nullptr;

  if (!mCachedNextEntry)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*_retval = mCachedNextEntry);
  
  mCachedNextEntry = nullptr;

  return NS_OK;
}
