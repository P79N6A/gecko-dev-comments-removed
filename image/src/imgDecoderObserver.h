




#ifndef MOZILLA_IMAGELIB_IMGDECODEROBSERVER_H_
#define MOZILLA_IMAGELIB_IMGDECODEROBSERVER_H_

#include "nsRect.h"
#include "mozilla/RefPtr.h"
#include "mozilla/WeakPtr.h"























class imgDecoderObserver : public mozilla::RefCounted<imgDecoderObserver>
{
public:
  virtual ~imgDecoderObserver() = 0;

  






  virtual void OnStartRequest() = 0;

  






  virtual void OnStartDecode() = 0;

  







  virtual void OnStartContainer() = 0;

  




  virtual void OnDataAvailable(const nsIntRect * aRect) = 0;

  virtual void FrameChanged(const nsIntRect * aDirtyRect) = 0;

  




  virtual void OnStopFrame() = 0;

  



  virtual void OnImageIsAnimated() = 0;

  




  virtual void OnStopDecode(nsresult status) = 0;

  






  virtual void OnStopRequest(bool aIsLastPart) = 0;

  




  virtual void OnDiscard() = 0;

  


  virtual void OnUnlockedDraw() = 0;
};




inline imgDecoderObserver::~imgDecoderObserver()
{}

#endif 
