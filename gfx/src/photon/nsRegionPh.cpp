




































#include "nsRegionPh.h"


#undef DEBUG
#undef FORCE_PR_LOG

#include "nsPhGfxLog.h"


#define tulx t->rect.ul.x
#define tuly t->rect.ul.y
#define tlrx t->rect.lr.x
#define tlry t->rect.lr.y

#define culx c->rect.ul.x
#define culy c->rect.ul.y
#define clrx c->rect.lr.x
#define clry c->rect.lr.y

NS_IMPL_ISUPPORTS1(nsRegionPh, nsIRegion)


void nsRegionPh :: Intersect( PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight ) 
{
	if( aWidth > 0 && aHeight > 0 ) {
		
		PhTile_t tile;
		tile.rect.ul.x = aX;
		tile.rect.ul.y = aY;
		tile.rect.lr.x = (aX+aWidth-1);
		tile.rect.lr.y = (aY+aHeight-1);
		tile.next = NULL;
    
		PhTile_t *original = mRegion;
		mRegion = PhIntersectTilings( mRegion, &tile, NULL );
		PhFreeTiles( original );
		if ( mRegion == NULL )
			SetTo(0, 0, 1, 1);
	}
	else 
		SetRegionEmpty();
}


PRBool nsRegionPh :: IsEqual( const nsIRegion &aRegion ) {
  PRBool result = PR_TRUE;
  PhTile_t *tiles;
  aRegion.GetNativeRegion((void*&)tiles);

  
  if( mRegion == tiles ) return PR_TRUE;
  else if( mRegion == NULL || tiles == NULL ) return PR_FALSE;
    
  PhSortTiles( mRegion );
  PhSortTiles( tiles );

  PhTile_t *t = mRegion, *c = tiles;
  while( t ) {
    if( tulx != culx || tuly != culy || tlrx != clrx || tlry != clry ) {
      result = PR_FALSE;
      break;	
			}
    t = t->next;  
    c = c->next;
  	}
  return result;
	}


void nsRegionPh :: GetBoundingBox( PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight ) {

	
	if( mRegion && !mRegion->next ) {
		*aX = mRegion->rect.ul.x;
		*aY = mRegion->rect.ul.y;
		*aWidth = mRegion->rect.lr.x - mRegion->rect.ul.x + 1;
		*aHeight = mRegion->rect.lr.y - mRegion->rect.ul.y + 1;
		return;
		}

  int bX=-32767, bY=-32767;

  *aX = 32767; 
  *aY = 32767; 

	PhTile_t *t = mRegion;

	while( t ) {
		*aX = PR_MIN( *aX, tulx );
		*aY = PR_MIN( *aY, tuly );
		bX = PR_MAX( bX, tlrx );
		bY = PR_MAX( bY, tlry );	 
		t = t->next;   
		}

	*aWidth =  bX - *aX + 1;
	*aHeight = bY - *aY + 1;
	}


PRBool nsRegionPh :: ContainsRect( PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight ) {
  if( !mRegion ) return PR_FALSE;

	PhCoalesceTiles( PhMergeTiles( PhSortTiles( mRegion )));  

	
	PhTile_t *tile = PhGetTile();
	tile->rect.ul.x = aX;
	tile->rect.ul.y = aY;
	tile->rect.lr.x = (aX+aWidth-1);
	tile->rect.lr.y = (aY+aHeight-1);
	tile->next = NULL;

	PhTile_t *test;
	test = PhIntersectTilings( tile, mRegion, NULL );

	PhFreeTiles( tile );

	if( test ) {
		PhFreeTiles( test );
		return PR_TRUE;
		}
	else return PR_FALSE;
	}


NS_IMETHODIMP nsRegionPh :: GetRects( nsRegionRectSet **aRects ) {

	
	if( mRegion && !mRegion->next ) {
		if( *aRects == nsnull ) *aRects = ( nsRegionRectSet * ) PR_Malloc( sizeof( nsRegionRectSet ) );
		nsRegionRect *rect = (*aRects)->mRects;
		(*aRects)->mRectsLen = (*aRects)->mNumRects = 1;
		rect->x = mRegion->rect.ul.x;
		rect->y = mRegion->rect.ul.y;
		rect->width = mRegion->rect.lr.x - mRegion->rect.ul.x + 1;
		rect->height = mRegion->rect.lr.y - mRegion->rect.ul.y + 1;
		(*aRects)->mArea = rect->width * rect->height;
		return NS_OK;
		}

	
  nsRegionRectSet   *rects;
  int               nbox = 0;
  nsRegionRect      *rect;
  PhTile_t	        *t = mRegion;

  while( t ) { nbox++; t = t->next; } 

  rects = *aRects;

  if ((nsnull == rects) || (rects->mRectsLen < (PRUint32) nbox)) {
    void *buf = PR_Realloc(rects, sizeof(nsRegionRectSet) + (sizeof(nsRegionRect) * (nbox - 1)));
    if (nsnull == buf) {
      if (nsnull != rects) rects->mNumRects = 0;
      return NS_OK;
    	}
		  
    rects = (nsRegionRectSet *) buf;
    rects->mRectsLen = nbox;
  	}

  rects->mNumRects = nbox;
  rects->mArea = 0;
  rect = &rects->mRects[0];
  t = mRegion;                  

  while( nbox-- ) {
    rect->x = tulx;
    rect->width = (tlrx - tulx+1);
		rect->y = tuly;
    rect->height = (tlry - tuly+1);											  
    rects->mArea += rect->width * rect->height;
    rect++;
    t = t->next;
  	}
 
  *aRects = rects;
  return NS_OK;
	}
