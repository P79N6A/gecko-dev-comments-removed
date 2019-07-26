




#ifndef ImageEncoder_h
#define ImageEncoder_h

#include "imgIEncoder.h"
#include "nsDOMFile.h"
#include "nsError.h"
#include "mozilla/dom/HTMLCanvasElementBinding.h"
#include "nsLayoutUtils.h"
#include "nsNetUtil.h"
#include "nsSize.h"

class nsICanvasRenderingContextInternal;

namespace mozilla {
namespace dom {

class EncodingRunnable;

class ImageEncoder
{
public:
  
  
  
  
  
  
  static nsresult ExtractData(nsAString& aType,
                              const nsAString& aOptions,
                              const nsIntSize aSize,
                              nsICanvasRenderingContextInternal* aContext,
                              nsIInputStream** aStream);

  
  
  
  
  
  
  
  static nsresult ExtractDataAsync(nsAString& aType,
                                   const nsAString& aOptions,
                                   bool aUsingCustomOptions,
                                   uint8_t* aImageBuffer,
                                   int32_t aFormat,
                                   const nsIntSize aSize,
                                   nsICanvasRenderingContextInternal* aContext,
                                   nsIScriptContext* aScriptContext,
                                   FileCallback& aCallback);

  
  
  
  static nsresult GetInputStream(int32_t aWidth,
                                 int32_t aHeight,
                                 uint8_t* aImageBuffer,
                                 int32_t aFormat,
                                 imgIEncoder* aEncoder,
                                 const PRUnichar* aEncoderOptions,
                                 nsIInputStream** aStream);

private:
  
  static nsresult
  ExtractDataInternal(const nsAString& aType,
                      const nsAString& aOptions,
                      uint8_t* aImageBuffer,
                      int32_t aFormat,
                      const nsIntSize aSize,
                      nsICanvasRenderingContextInternal* aContext,
                      nsIInputStream** aStream,
                      imgIEncoder* aEncoder);

  
  
  
  
  
  static already_AddRefed<imgIEncoder> GetImageEncoder(nsAString& aType);

  friend class EncodingRunnable;
};

} 
} 

#endif 