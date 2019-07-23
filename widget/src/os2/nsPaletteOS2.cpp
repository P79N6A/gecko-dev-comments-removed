



























#include "nsGfxDefs.h"
#include <stdlib.h>

#include "nsDeviceContextOS2.h" 
#include "nsPaletteOS2.h"
#include "il_util.h"











class nsPaletteOS2 : public nsIPaletteOS2
{
 protected:
   nsIDeviceContext *mContext; 
   PRUint8          *mGammaTable;

 public:
   virtual nsresult Init( nsIDeviceContext *aContext,
                          ULONG * = 0, ULONG = 0)
   {
      mContext = aContext;
      mContext->GetGammaTable( mGammaTable);
      return mContext == nsnull ? NS_ERROR_FAILURE : NS_OK;
   }

   long GetGPIColor( nsIDeviceContext *aContext, HPS hps, nscolor rgb)
   {
      if (mContext != aContext) {
         mContext = aContext;
         mContext->GetGammaTable( mGammaTable);
      }
      long gcolor = MK_RGB( mGammaTable[NS_GET_R(rgb)],
                            mGammaTable[NS_GET_G(rgb)],
                            mGammaTable[NS_GET_B(rgb)]);
      return GpiQueryColorIndex( hps, 0, gcolor);
   }

   virtual nsresult GetNSPalette( nsPalette &aPalette) const
   {
      aPalette = 0;
      return NS_OK;
   }

   NS_DECL_ISUPPORTS

   nsPaletteOS2()
   {
      NS_INIT_REFCNT();
      mContext = nsnull;
      mGammaTable = 0;
   }

   virtual ~nsPaletteOS2()
   {}
};


nsresult nsPaletteOS2::QueryInterface( const nsIID&, void**)
{
   return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsPaletteOS2)
NS_IMPL_RELEASE(nsPaletteOS2)


class nsLCOLPaletteOS2 : public nsPaletteOS2
{
   ULONG *mTable;
   ULONG  mSize;

 public:
   nsresult Init( nsIDeviceContext *aContext,
                  ULONG *pEntries, ULONG cEntries)
   {
      mTable = pEntries;
      mSize = cEntries;
      return nsPaletteOS2::Init( aContext);
   }

   nsresult Select( HPS hps, nsIDeviceContext *)
   {
      BOOL rc = GpiCreateLogColorTable( hps, LCOL_RESET | LCOL_PURECOLOR,
                                        LCOLF_CONSECRGB, 0,
                                        mSize, (PLONG) mTable);
      if( !rc)
         PMERROR( "GpiCreateLogColorTable");
      return rc ? NS_OK : NS_ERROR_FAILURE;
   }

   nsresult Deselect( HPS hps)
   {
      BOOL rc = GpiCreateLogColorTable( hps, LCOL_RESET, 0, 0, 0, 0);
      return rc ? NS_OK : NS_ERROR_FAILURE;
   }

   nsLCOLPaletteOS2()
   {
      mTable = 0;
      mSize = 0;
   }

  ~nsLCOLPaletteOS2()
   {
      if( mTable) free( mTable);
   }
};


class nsHPALPaletteOS2 : public nsPaletteOS2
{
   HPAL mHPal;

 public:
   nsresult Init( nsIDeviceContext *aContext,
                  ULONG *pEntries, ULONG cEntries)
   {
      mHPal = GpiCreatePalette( 0, LCOL_PURECOLOR, LCOLF_CONSECRGB,
                                cEntries, pEntries);
      free( pEntries);

      return nsPaletteOS2::Init( aContext);
   }

   nsresult GetNSPalette( nsPalette &aPalette) const
   {
      aPalette = (nsPalette) mHPal;
      return NS_OK;
   }

   nsresult Select( HPS hps, nsIDeviceContext *aContext)
   {
      HPAL rc = GpiSelectPalette( hps, mHPal);
      if( rc == (HPAL) PAL_ERROR)
      {
         PMERROR( "GpiSelectPalette");
         return NS_ERROR_FAILURE;
      }

      
      
      nsNativeWidget wdg = ((nsDeviceContextOS2 *) aContext)->mWidget;
      if( wdg)
      {
         ULONG ulDummy = 0;
         WinRealizePalette( (HWND)wdg, hps, &ulDummy);
      }
      return NS_OK;
   }

   nsresult Deselect( HPS hps)
   {
      HPAL rc = GpiSelectPalette( hps, 0);
      return rc == ((HPAL)PAL_ERROR) ? NS_ERROR_FAILURE : NS_OK;
   }

   nsHPALPaletteOS2()
   {
      mHPal = 0;
   }

  ~nsHPALPaletteOS2()
   {
      if( mHPal)
         GpiDeletePalette( mHPal);
   }
};


class nsRGBPaletteOS2 : public nsPaletteOS2
{
 public:
   nsresult Select( HPS hps, nsIDeviceContext *)
   {
      BOOL rc = GpiCreateLogColorTable( hps, LCOL_PURECOLOR,
                                        LCOLF_RGB, 0, 0, 0);
      if( !rc)
         PMERROR( "GpiCreateLogColorTable #2");
      return rc ? NS_OK : NS_ERROR_FAILURE;
   }

   nsresult Deselect( HPS hps)
   {
      BOOL rc = GpiCreateLogColorTable( hps, LCOL_RESET, 0, 0, 0, 0);
      return rc ? NS_OK : NS_ERROR_FAILURE;
   }

   nsRGBPaletteOS2() {}
  ~nsRGBPaletteOS2() {}
};

nsresult NS_CreatePalette( nsIDeviceContext *aContext, nsIPaletteOS2 *&aPalette)
{
   nsresult       rc = NS_OK;
   IL_ColorSpace *colorSpace = 0;

   nsPaletteOS2 *newPalette = 0;

   rc = aContext->GetILColorSpace( colorSpace);
   if( NS_SUCCEEDED(rc))
   {
      if( NI_PseudoColor == colorSpace->type)
      {
         PULONG pPalette = (PULONG) calloc( COLOR_CUBE_SIZE, sizeof( ULONG));
   
         
         for( PRInt32 i = 0; i < COLOR_CUBE_SIZE; i++)
         {
            IL_RGB *map = colorSpace->cmap.map + i;
            pPalette[ i] = MK_RGB( map->red, map->green, map->blue);
         }
   
         
         
         if( getenv( "MOZ_USE_LCOL"))
            newPalette = new nsLCOLPaletteOS2;
         else
            newPalette = new nsHPALPaletteOS2;
         rc = newPalette->Init( aContext, pPalette, COLOR_CUBE_SIZE);
      }
      else
      {
         newPalette = new nsRGBPaletteOS2;
         rc = newPalette->Init( aContext);
      }
   
      IL_ReleaseColorSpace( colorSpace);
   }

   if( NS_SUCCEEDED(rc))
   {
      NS_ADDREF(newPalette);
      aPalette = newPalette;
   }

   return rc;
}
