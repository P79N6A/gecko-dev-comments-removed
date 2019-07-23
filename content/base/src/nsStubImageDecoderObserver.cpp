




































#include "nsStubImageDecoderObserver.h"

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStartRequest(imgIRequest *aRequest)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStartDecode(imgIRequest *aRequest)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStartContainer(imgIRequest *aRequest,
                                             imgIContainer *aContainer)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStartFrame(imgIRequest *aRequest,
                                         gfxIImageFrame *aFrame)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnDataAvailable(imgIRequest *aRequest,
                                            gfxIImageFrame *aFrame,
                                            const nsIntRect * aRect)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStopFrame(imgIRequest *aRequest,
                                        gfxIImageFrame *aFrame)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStopContainer(imgIRequest *aRequest,
                                            imgIContainer *aContainer)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStopDecode(imgIRequest *aRequest,
                                         nsresult status,
                                         const PRUnichar *statusArg)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::OnStopRequest(imgIRequest *aRequest, 
                                          PRBool aIsLastPart)
{
    return NS_OK;
}

NS_IMETHODIMP
nsStubImageDecoderObserver::FrameChanged(imgIContainer *aContainer,
                                         gfxIImageFrame *aFrame,
                                         nsIntRect * aDirtyRect)
{
    return NS_OK;
}
