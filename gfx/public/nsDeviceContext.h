





































#ifndef nsDeviceContext_h___
#define nsDeviceContext_h___

#include "nsIDeviceContext.h"
#include "nsIDeviceContextSpec.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsVoidArray.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "gfxCore.h"

class nsIImageRequest;
class nsHashtable;

class NS_GFX nsFontCache
{
public:
  nsFontCache();
  virtual ~nsFontCache();

  virtual nsresult Init(nsIDeviceContext* aContext);
  virtual nsresult GetDeviceContext(nsIDeviceContext *&aContext) const;
  virtual nsresult GetMetricsFor(const nsFont& aFont, nsIAtom* aLangGroup,
                                 nsIFontMetrics *&aMetrics);

  nsresult   FontMetricsDeleted(const nsIFontMetrics* aFontMetrics);
  nsresult   Compact();
  nsresult   Flush();
  



  virtual nsresult CreateFontMetricsInstance(nsIFontMetrics** fm);
  
protected:
  nsVoidArray      mFontMetrics;
  nsIDeviceContext *mContext; 
                              
};


#undef IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY

class NS_GFX DeviceContextImpl : public nsIDeviceContext,
                                 public nsIObserver,
                                 public nsSupportsWeakReference
{
public:
  DeviceContextImpl();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  NS_IMETHOD  Init(nsNativeWidget aWidget);

  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext){return NS_ERROR_NOT_IMPLEMENTED;}
  NS_IMETHOD  CreateRenderingContext(nsIDrawingSurface* aSurface, nsIRenderingContext *&aContext);
  NS_IMETHOD  CreateRenderingContextInstance(nsIRenderingContext *&aContext);

  NS_IMETHOD  GetMetricsFor(const nsFont& aFont, nsIAtom* aLangGroup,
                            nsIFontMetrics*& aMetrics);
  NS_IMETHOD  GetMetricsFor(const nsFont& aFont, nsIFontMetrics*& aMetrics);

  NS_IMETHOD FirstExistingFont(const nsFont& aFont, nsString& aFaceName);

  NS_IMETHOD GetLocalFontName(const nsString& aFaceName, nsString& aLocalName,
                              PRBool& aAliased);

  NS_IMETHOD CreateFontCache();
  NS_IMETHOD FontMetricsDeleted(const nsIFontMetrics* aFontMetrics);
  NS_IMETHOD FlushFontCache(void);

  NS_IMETHOD GetDepth(PRUint32& aDepth);

  NS_IMETHOD GetPaletteInfo(nsPaletteInfo& aPaletteInfo);

  NS_IMETHOD PrepareDocument(PRUnichar * aTitle, 
                             PRUnichar*  aPrintToFileName) { return NS_OK; }
  NS_IMETHOD AbortDocument(void) { return NS_OK; }

  NS_IMETHOD PrepareNativeWidget(nsIWidget *aWidget, void **aOut);
  NS_IMETHOD ClearCachedSystemFonts();

private:
  
  nsresult InitRenderingContext(nsIRenderingContext *aContext, nsIWidget *aWindow);
  nsresult InitRenderingContext(nsIRenderingContext *aContext, nsIDrawingSurface* aSurface);

protected:
  virtual ~DeviceContextImpl();

  void CommonInit(void);
  nsresult CreateIconILGroupContext();
  virtual nsresult CreateFontAliasTable();
  nsresult AliasFont(const nsString& aFont, 
                     const nsString& aAlias, const nsString& aAltAlias,
                     PRBool aForceAlias);
  void GetLocaleLangGroup(void);

  nsFontCache       *mFontCache;
  nsCOMPtr<nsIAtom> mLocaleLangGroup; 
  nsHashtable*      mFontAliasTable;

public:
  nsNativeWidget    mWidget;
#ifdef NS_DEBUG
  PRBool            mInitialized;
#endif
};

#undef IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_HIDDEN

#endif 
