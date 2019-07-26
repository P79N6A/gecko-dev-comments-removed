




#ifndef _nsTextAccessibleWrap_H_
#define _nsTextAccessibleWrap_H_

#include "nsTextAccessible.h"
#include "ISimpleDOMText.h"
#include "nsRect.h"

class nsIFrame;
class nsRenderingContext;

class nsTextAccessibleWrap : public nsTextAccessible, 
                             public ISimpleDOMText
{
public:
  nsTextAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~nsTextAccessibleWrap() {}

    
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

#endif

