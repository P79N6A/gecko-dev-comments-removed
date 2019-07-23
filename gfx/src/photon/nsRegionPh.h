




































#ifndef nsRegionPh_h___
#define nsRegionPh_h___

#include "nsIRegion.h"
#include "prmem.h"
#include <Pt.h>

class nsRegionPh : public nsIRegion
{
public:
  inline nsRegionPh()
		{
		mRegion = NULL;
		mRegionType = eRegionComplexity_empty;
		}

  inline nsRegionPh(PhTile_t *tiles)
		{
		mRegion = tiles; 
		mRegionType = (mRegion == NULL) ? eRegionComplexity_empty : eRegionComplexity_complex;
		}

  virtual ~nsRegionPh()
		{
		if( mRegion ) PhFreeTiles( mRegion );
		mRegion = nsnull;
		}

  NS_DECL_ISUPPORTS

  virtual nsresult Init()
		{
		SetRegionEmpty();
		return NS_OK;
		}

  virtual void SetTo(const nsIRegion &aRegion)
		{
		PhTile_t *tiles;
		aRegion.GetNativeRegion( ( void*& ) tiles );
		SetRegionEmpty( );
		mRegion = PhCopyTiles( tiles );
		}

  virtual void SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
		{
  	SetRegionEmpty( );

  	if ( aWidth > 0 && aHeight > 0 ) {
  	  
  	  PhTile_t *tile = PhGetTile( );
  	  tile->rect.ul.x = aX;
  	  tile->rect.ul.y = aY;
  	  tile->rect.lr.x = (aX+aWidth-1);
  	  tile->rect.lr.y = (aY+aHeight-1);
  	  tile->next = NULL;
  	  mRegion = tile;
  	  }
		}

  virtual void Intersect(const nsIRegion &aRegion)
		{
  	PhTile_t *original = mRegion;
  	PhTile_t *tiles;
  	aRegion.GetNativeRegion( ( void*& ) tiles );
  	mRegion = PhIntersectTilings( original, tiles, NULL);
  	if( mRegion )
  	  mRegion = PhCoalesceTiles( PhMergeTiles( PhSortTiles( mRegion )));
  	PhFreeTiles( original );
  	if ( mRegion == NULL )
  	  SetTo(0, 0, 1, 1);
		}

  virtual void Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);

  virtual void Union(const nsIRegion &aRegion)
		{
		PhTile_t *tiles;
		aRegion.GetNativeRegion( ( void*& ) tiles );
		mRegion = PhAddMergeTiles( mRegion, PhCopyTiles( tiles ), NULL );
		}

  virtual void Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
		{
		if( aWidth > 0 && aHeight > 0 ) {
		  
		  PhTile_t *tile = PhGetTile();
		  tile->rect.ul.x = aX;
		  tile->rect.ul.y = aY;
		  tile->rect.lr.x = (aX+aWidth-1);
		  tile->rect.lr.y = (aY+aHeight-1);
		  tile->next = NULL;

		  mRegion = PhAddMergeTiles( mRegion, tile, NULL );
		  }
		}

  virtual void Subtract(const nsIRegion &aRegion)
		{
		PhTile_t *tiles;
		aRegion.GetNativeRegion((void*&)tiles);
		mRegion = PhClipTilings( mRegion, tiles, NULL );
		}

  virtual void Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
		{
		if( aWidth > 0 && aHeight > 0 ) {
		  
		  PhTile_t tile;
		  tile.rect.ul.x = aX;
		  tile.rect.ul.y = aY;
		  tile.rect.lr.x = aX + aWidth - 1;
		  tile.rect.lr.y = aY + aHeight - 1;
		  tile.next = NULL;

		  mRegion = PhClipTilings( mRegion, &tile, NULL );
			}
		}

  virtual PRBool IsEmpty(void)
		{
  		if ( !mRegion )
  		  return PR_TRUE;
  		if ( mRegion->rect.ul.x == 0 && mRegion->rect.ul.y == 0 &&
  		  mRegion->rect.lr.x == 0 && mRegion->rect.lr.y == 0 )
  		  return PR_TRUE;
  		return PR_FALSE;
		}

  virtual PRBool IsEqual(const nsIRegion &aRegion);
  virtual void GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight);

  virtual void Offset(PRInt32 aXOffset, PRInt32 aYOffset)
		{
  	if( ( aXOffset || aYOffset ) && mRegion ) {
  	  PhPoint_t p = { aXOffset, aYOffset };

  	  
  	  if( !mRegion->next ) PtTranslateRect( &mRegion->rect, &p );
  	  else PhTranslateTiles( mRegion, &p );
  	  }
		}

  virtual PRBool ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD GetRects(nsRegionRectSet **aRects);

  inline NS_IMETHODIMP FreeRects(nsRegionRectSet *aRects)
		{
		if( nsnull != aRects ) PR_Free( ( void * )aRects );
		return NS_OK;
		}

  inline NS_IMETHODIMP GetNativeRegion(void *&aRegion) const
		{
		aRegion = (void *) mRegion;
		return NS_OK;
		}

  inline NS_IMETHODIMP GetRegionComplexity(nsRegionComplexity &aComplexity) const
		{
		aComplexity = mRegionType;
		return NS_OK;
		}

  inline NS_IMETHOD GetNumRects(PRUint32 *aRects) const { *aRects = 0; return NS_OK; }

private:
  virtual void SetRegionEmpty()
		{
		if( mRegion ) PhFreeTiles( mRegion );
		mRegion = NULL;
		mRegionType = eRegionComplexity_empty;
		}

  PhTile_t             *mRegion;
  nsRegionComplexity    mRegionType;		
};

#endif  
