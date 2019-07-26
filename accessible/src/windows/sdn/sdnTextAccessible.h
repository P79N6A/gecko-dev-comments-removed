





#ifndef mozilla_a11y_sdnTextAccessible_h_
#define mozilla_a11y_sdnTextAccessible_h_

#include "ISimpleDOMText.h"
#include "IUnknownImpl.h"

#include "AccessibleWrap.h"

class nsIFrame;
class nsPoint;

namespace mozilla {
namespace a11y {
 
class sdnTextAccessible MOZ_FINAL : public ISimpleDOMText
{
public:
  sdnTextAccessible(AccessibleWrap* aAccessible) : mAccessible(aAccessible) {};
  ~sdnTextAccessible() {}

  DECL_IUNKNOWN

  

  virtual  HRESULT STDMETHODCALLTYPE get_domText(
     BSTR __RPC_FAR *aText);

  virtual HRESULT STDMETHODCALLTYPE get_clippedSubstringBounds(
     unsigned int startIndex,
     unsigned int endIndex,
     int __RPC_FAR* aX,
     int __RPC_FAR* aY,
     int __RPC_FAR* aWidth,
     int __RPC_FAR* aHeight);

  virtual HRESULT STDMETHODCALLTYPE get_unclippedSubstringBounds(
     unsigned int aStartIndex,
     unsigned int aEndIndex,
     int __RPC_FAR* aX,
     int __RPC_FAR* aY,
     int __RPC_FAR* aWidth,
     int __RPC_FAR* aHeight);

  virtual HRESULT STDMETHODCALLTYPE scrollToSubstring(
     unsigned int aStartIndex,
     unsigned int aEndIndex);

  virtual  HRESULT STDMETHODCALLTYPE get_fontFamily(
     BSTR __RPC_FAR* aFontFamily);

private:
  


  nsIFrame* GetPointFromOffset(nsIFrame* aContainingFrame,
                               int32_t aOffset, bool aPreferNext,
                               nsPoint& aOutPoint);

  nsRefPtr<AccessibleWrap> mAccessible;
};

} 
} 

#endif
