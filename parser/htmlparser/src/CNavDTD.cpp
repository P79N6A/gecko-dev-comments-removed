





#include "mozilla/Util.h"

#include "nsISupports.h"
#include "nsISupportsImpl.h"
#include "nsIParser.h"
#include "CNavDTD.h"
#include "nsIHTMLContentSink.h"

NS_IMPL_ISUPPORTS1(CNavDTD, nsIDTD);

CNavDTD::CNavDTD()
{
}

CNavDTD::~CNavDTD()
{
}

NS_IMETHODIMP
CNavDTD::WillBuildModel(const CParserContext& aParserContext,
                        nsITokenizer* aTokenizer,
                        nsIContentSink* aSink)
{
  return NS_OK;
}

NS_IMETHODIMP
CNavDTD::BuildModel(nsITokenizer* aTokenizer,
                    nsIContentSink* aSink)
{
  
  
  nsCOMPtr<nsIHTMLContentSink> sink = do_QueryInterface(aSink);
  if (!sink) {
    return NS_ERROR_HTMLPARSER_STOPPARSING;
  }

  nsresult rv = sink->OpenContainer(eHTMLTag_html);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = sink->OpenContainer(eHTMLTag_body);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sink->CloseContainer(eHTMLTag_body);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  rv = sink->CloseContainer(eHTMLTag_html);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  return NS_OK;
}

NS_IMETHODIMP
CNavDTD::DidBuildModel(nsresult anErrorCode)
{
  return NS_OK;
}

NS_IMETHODIMP_(void)
CNavDTD::Terminate()
{
}


NS_IMETHODIMP_(int32_t) 
CNavDTD::GetType() 
{ 
  return NS_IPARSER_FLAG_HTML; 
}

NS_IMETHODIMP_(nsDTDMode)
CNavDTD::GetMode() const
{
  return eDTDMode_quirks;
}

NS_IMETHODIMP_(bool)
CNavDTD::CanContain(int32_t aParent,int32_t aChild) const
{
  MOZ_CRASH("nobody calls this");
  return false;
}

NS_IMETHODIMP_(bool)
CNavDTD::IsContainer(int32_t aTag) const
{
  MOZ_CRASH("nobody calls this");
  return false;
}
