




































#include <nsIPipe.h>
#include <nsIInputStream.h>
#include <nsIOutputStream.h>
#include <nsIContentViewerContainer.h>
#include <nsIDocumentLoaderFactory.h>
#include <nsNetUtil.h>
#include <prmem.h>

#include "nsXPCOMCID.h"
#include "nsICategoryManager.h"

#include "nsIContentViewer.h"

#include "nsEmbedStream.h"
#include "nsReadableUtils.h"



NS_IMPL_ISUPPORTS1(nsEmbedStream, nsIInputStream)

nsEmbedStream::nsEmbedStream()
{
  mOwner       = nsnull;
  mOffset      = 0;
  mDoingStream = PR_FALSE;
}

nsEmbedStream::~nsEmbedStream()
{
}

void
nsEmbedStream::InitOwner(nsIWebBrowser *aOwner)
{
  mOwner = aOwner;
}

NS_METHOD
nsEmbedStream::Init(void)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIInputStream> bufInStream;
  nsCOMPtr<nsIOutputStream> bufOutStream;

  rv = NS_NewPipe(getter_AddRefs(bufInStream),
                  getter_AddRefs(bufOutStream));

  if (NS_FAILED(rv)) return rv;
 
  mInputStream  = bufInStream;
  mOutputStream = bufOutStream;

  return NS_OK;
}

NS_METHOD
nsEmbedStream::OpenStream(nsIURI *aBaseURI, const nsACString& aContentType)
{
  NS_ENSURE_ARG_POINTER(aBaseURI);
  NS_ENSURE_TRUE(IsASCII(aContentType), NS_ERROR_INVALID_ARG);

  
  if (mDoingStream)
    return NS_ERROR_IN_PROGRESS;

  
  mDoingStream = PR_TRUE;

  
  nsresult rv = Init();
  if (NS_FAILED(rv))
    return rv;

  
  nsCOMPtr<nsIContentViewerContainer> viewerContainer;
  viewerContainer = do_GetInterface(mOwner);

  
  rv = NS_NewLoadGroup(getter_AddRefs(mLoadGroup), nsnull);
  if (NS_FAILED(rv))
    return rv;

  
  rv = NS_NewInputStreamChannel(getter_AddRefs(mChannel), aBaseURI,
				static_cast<nsIInputStream *>(this),
				aContentType);
  if (NS_FAILED(rv))
    return rv;

  
  rv = mChannel->SetLoadGroup(mLoadGroup);
  if (NS_FAILED(rv))
    return rv;

  

  const nsCString& flatContentType = PromiseFlatCString(aContentType);

  nsXPIDLCString docLoaderContractID;
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  rv = catMan->GetCategoryEntry("Gecko-Content-Viewers",
                                flatContentType.get(),
                                getter_Copies(docLoaderContractID));
  
  
  
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory;  
  docLoaderFactory = do_GetService(docLoaderContractID.get(), &rv);
  if (NS_FAILED(rv))
    return rv;

  
  
  nsCOMPtr<nsIContentViewer> contentViewer;
  rv = docLoaderFactory->CreateInstance("view", mChannel, mLoadGroup,
					flatContentType.get(), viewerContainer,
					nsnull,
					getter_AddRefs(mStreamListener),
					getter_AddRefs(contentViewer));
  if (NS_FAILED(rv))
    return rv;

  
  rv = contentViewer->SetContainer(viewerContainer);
  if (NS_FAILED(rv))
    return rv;

  
  rv = viewerContainer->Embed(contentViewer, "view", nsnull);
  if (NS_FAILED(rv))
    return rv;

  
  rv = mStreamListener->OnStartRequest(mChannel, NULL);
  if (NS_FAILED(rv))
    return rv;
  
  return NS_OK;
}

NS_METHOD
nsEmbedStream::AppendToStream(const PRUint8 *aData, PRUint32 aLen)
{
  nsresult rv;

  
  rv = Append(aData, aLen);
  if (NS_FAILED(rv))
    return rv;

  
  rv = mStreamListener->OnDataAvailable(mChannel,
					NULL,
					static_cast<nsIInputStream *>(this),
					mOffset, 
					aLen); 
  
  mOffset += aLen;
  if (NS_FAILED(rv))
    return rv;

  return NS_OK;
}

NS_METHOD
nsEmbedStream::CloseStream(void)
{
  nsresult rv = NS_OK;

  
  
  NS_ENSURE_STATE(mDoingStream);
  mDoingStream = PR_FALSE;

  rv = mStreamListener->OnStopRequest(mChannel, NULL, NS_OK);
  if (NS_FAILED(rv))
    return rv;

  mLoadGroup = nsnull;
  mChannel = nsnull;
  mStreamListener = nsnull;
  mOffset = 0;

  return rv;
}

NS_METHOD
nsEmbedStream::Append(const PRUint8 *aData, PRUint32 aLen)
{
  PRUint32 bytesWritten = 0;
  nsresult rv = mOutputStream->Write(reinterpret_cast<const char*>(aData),
                                     aLen, &bytesWritten);
  if (NS_FAILED(rv))
    return rv;
  
  NS_ASSERTION(bytesWritten == aLen,
	       "underlying byffer couldn't handle the write");
  return rv;
}

NS_IMETHODIMP
nsEmbedStream::Available(PRUint32 *_retval)
{
  return mInputStream->Available(_retval);
}

NS_IMETHODIMP
nsEmbedStream::Read(char * aBuf, PRUint32 aCount, PRUint32 *_retval)
{
  return mInputStream->Read(aBuf, aCount, _retval);
}

NS_IMETHODIMP nsEmbedStream::Close(void)
{
  return mInputStream->Close();
}

NS_IMETHODIMP
nsEmbedStream::ReadSegments(nsWriteSegmentFun aWriter, void * aClosure,
			    PRUint32 aCount, PRUint32 *_retval)
{
  return mInputStream->ReadSegments(aWriter, aClosure, aCount, _retval);
}

NS_IMETHODIMP
nsEmbedStream::IsNonBlocking(PRBool *aNonBlocking)
{
    return mInputStream->IsNonBlocking(aNonBlocking);
}
