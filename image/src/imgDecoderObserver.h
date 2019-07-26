




#ifndef MOZILLA_IMAGELIB_IMGDECODEROBSERVER_H_
#define MOZILLA_IMAGELIB_IMGDECODEROBSERVER_H_

#include "nsRect.h"
#include "mozilla/WeakPtr.h"

struct nsIntRect;























class imgDecoderObserver
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(imgDecoderObserver);

  






  virtual void OnStartRequest() = 0;

  






  virtual void OnStartDecode() = 0;

  







  virtual void OnStartContainer() = 0;

  




  virtual void OnStartFrame() = 0;

  




  virtual void FrameChanged(const nsIntRect * aDirtyRect) = 0;

  




  virtual void OnStopFrame() = 0;

  



  virtual void OnImageIsAnimated() = 0;

  




  virtual void OnStopDecode(nsresult status) = 0;

  






  virtual void OnStopRequest(bool aIsLastPart, nsresult aStatus) = 0;

  




  virtual void OnDiscard() = 0;

  


  virtual void OnUnlockedDraw() = 0;

  


  virtual void OnError() = 0;

protected:
  virtual ~imgDecoderObserver() = 0;
};




inline imgDecoderObserver::~imgDecoderObserver()
{}

#endif 
