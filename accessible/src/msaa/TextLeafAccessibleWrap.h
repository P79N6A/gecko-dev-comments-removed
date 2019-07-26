




#ifndef mozilla_a11y_TextLeafAccessibleWrap_h__
#define mozilla_a11y_TextLeafAccessibleWrap_h__

#include "TextLeafAccessible.h"
#include "ISimpleDOMText.h"
#include "nsRect.h"

class nsIFrame;
class nsRenderingContext;

namespace mozilla {
namespace a11y {
 
class TextLeafAccessibleWrap : public TextLeafAccessible, 
                               public ISimpleDOMText
{
public:
  TextLeafAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~TextLeafAccessibleWrap() {}

    
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID, void**);

    virtual  HRESULT STDMETHODCALLTYPE get_domText( 
         BSTR __RPC_FAR *domText);
    
    virtual HRESULT STDMETHODCALLTYPE get_clippedSubstringBounds( 
         unsigned int startIndex,
         unsigned int endIndex,
         int __RPC_FAR *x,
         int __RPC_FAR *y,
         int __RPC_FAR *width,
         int __RPC_FAR *height);
    
    virtual HRESULT STDMETHODCALLTYPE get_unclippedSubstringBounds( 
         unsigned int startIndex,
         unsigned int endIndex,
         int __RPC_FAR *x,
         int __RPC_FAR *y,
         int __RPC_FAR *width,
         int __RPC_FAR *height);
    
    virtual HRESULT STDMETHODCALLTYPE scrollToSubstring( 
         unsigned int startIndex,
         unsigned int endIndex);

    virtual  HRESULT STDMETHODCALLTYPE get_fontFamily( 
         BSTR __RPC_FAR *fontFamily);
    
  protected:
    nsresult GetCharacterExtents(PRInt32 aStartOffset, PRInt32 aEndOffset,
                                 PRInt32* aX, PRInt32* aY, 
                                 PRInt32* aWidth, PRInt32* aHeight);

    
    nsIFrame* GetPointFromOffset(nsIFrame *aContainingFrame,
                                 PRInt32 aOffset, bool aPreferNext, nsPoint& aOutPoint);
};

} 
} 

#endif

