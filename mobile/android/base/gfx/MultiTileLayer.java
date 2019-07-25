




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.SingleTileLayer;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.util.Log;
import java.lang.Long;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.ListIterator;
import javax.microedition.khronos.opengles.GL10;






public class MultiTileLayer extends Layer {
    private static final String LOGTAG = "GeckoMultiTileLayer";

    private final CairoImage mImage;
    private final IntSize mTileSize;
    private IntSize mBufferSize;
    private Region mDirtyRegion;
    private Region mValidRegion;
    private Point mRenderOffset;
    private final LinkedList<SubTile> mTiles;
    private final HashMap<Long, SubTile> mPositionHash;

    
    
    private Point mOrigin;
    private float mResolution;

    public MultiTileLayer(CairoImage image, IntSize tileSize) {
        super();

        mImage = image;
        mTileSize = tileSize;
        mBufferSize = new IntSize(0, 0);
        mDirtyRegion = new Region();
        mValidRegion = new Region();
        mRenderOffset = new Point();
        mTiles = new LinkedList<SubTile>();
        mPositionHash = new HashMap<Long, SubTile>();
    }

    




    public void invalidate(Rect dirtyRect) {
        if (!inTransaction()) {
            throw new RuntimeException("invalidate() is only valid inside a transaction");
        }

        mDirtyRegion.union(dirtyRect);
        mValidRegion.union(dirtyRect);
    }

    



    protected void invalidateBuffer() {
        if (!inTransaction()) {
            throw new RuntimeException("invalidateBuffer() is only valid inside a transaction");
        }

        mDirtyRegion.setEmpty();
        mValidRegion.setEmpty();
    }

    @Override
    public IntSize getSize() {
        return mImage.getSize();
    }

    


    private void validateTiles() {
        IntSize size = getSize();

        if (size.equals(mBufferSize))
            return;

        mBufferSize = size;

        
        int nTiles = (Math.round(size.width / (float)mTileSize.width) + 1) *
                     (Math.round(size.height / (float)mTileSize.height) + 1);
        if (mTiles.size() < nTiles) {
            Log.i(LOGTAG, "Tile pool growing from " + mTiles.size() + " to " + nTiles);

            for (int i = 0; i < nTiles; i++) {
                mTiles.add(new SubTile(new SubImage(mImage, mTileSize)));
            }
        } else if (mTiles.size() > nTiles) {
            Log.i(LOGTAG, "Tile pool shrinking from " + mTiles.size() + " to " + nTiles);

            
            
            for (int i = mTiles.size(); i > nTiles; i--) {
                SubTile tile = mTiles.get(0);
                if (tile.key != null) {
                    mPositionHash.remove(tile.key);
                }
                mTiles.remove(0);
            }
        }

        
        invalidateTiles();
    }

    


    private Long longFromPoint(Point point) {
        
        return new Long((((long)point.x) << 32) | point.y);
    }

    private Point getOffsetOrigin() {
        Point origin = new Point(getOrigin());
        origin.offset(-mRenderOffset.x, -mRenderOffset.y);
        return origin;
    }

    



    private void updateTile(GL10 gl, RenderContext context, SubTile tile, Point tileOrigin, Rect dirtyRect, boolean reused) {
        tile.beginTransaction(null);
        try {
            if (reused) {
                
                
                
                
                Point origin = getOffsetOrigin();
                Region validRegion = new Region(tile.getValidTextureArea());
                validRegion.translate(tileOrigin.x - origin.x, tileOrigin.y - origin.y);
                validRegion.op(mValidRegion, Region.Op.INTERSECT);

                
                
                tile.invalidateTexture();
                if (!validRegion.isEmpty() && !validRegion.isComplex()) {
                    validRegion.translate(origin.x - tileOrigin.x, origin.y - tileOrigin.y);
                    tile.getValidTextureArea().set(validRegion.getBounds());
                }
            } else {
                
                tile.setOrigin(tileOrigin);
                tile.setResolution(getResolution());

                
                
                tile.invalidateTexture();

                
                if (tile.key != null) {
                    mPositionHash.remove(tile.key);
                }
                tile.key = longFromPoint(tileOrigin);
                mPositionHash.put(tile.key, tile);
            }

            
            tile.invalidate(dirtyRect);

            
            if (!tile.performUpdates(gl, context)) {
                Log.e(LOGTAG, "Sub-tile failed to update fully");
            }
        } finally {
            tile.endTransaction();
        }
    }

    @Override
    protected boolean performUpdates(GL10 gl, RenderContext context) {
        super.performUpdates(gl, context);

        validateTiles();

        
        if (mDirtyRegion.isEmpty() || mTiles.isEmpty()) {
            return true;
        }

        
        Point origin = getOffsetOrigin();
        if ((origin.x % mTileSize.width) != 0 || (origin.y % mTileSize.height) != 0) {
            Log.e(LOGTAG, "MultiTileLayer doesn't support non tile-aligned buffers! (" +
                  origin.x + ", " + origin.y + ")");
            return true;
        }

        
        
        
        
        
        
        
        
        Rect tilespaceViewport;
        float scaleFactor = getResolution() / context.zoomFactor;
        tilespaceViewport = RectUtils.roundOut(RectUtils.scale(context.viewport, scaleFactor));
        tilespaceViewport.offset(-origin.x, -origin.y);

        
        tilespaceViewport.left = (tilespaceViewport.left / mTileSize.width) * mTileSize.width;
        tilespaceViewport.right += mTileSize.width - 1;
        tilespaceViewport.right = (tilespaceViewport.right / mTileSize.width) * mTileSize.width;
        tilespaceViewport.top = (tilespaceViewport.top / mTileSize.height) * mTileSize.height;
        tilespaceViewport.bottom += mTileSize.height - 1;
        tilespaceViewport.bottom = (tilespaceViewport.bottom / mTileSize.height) * mTileSize.height;

        
        Region opRegion = new Region();

        
        boolean updateVisible = false;
        Region updateRegion = mDirtyRegion;
        if (opRegion.op(tilespaceViewport, mDirtyRegion, Region.Op.INTERSECT)) {
            updateVisible = true;
            updateRegion = new Region(opRegion);
        }

        
        
        
        
        
        
        
        
        
        
        
        
        LinkedList<SubTile> invalidTiles = new LinkedList<SubTile>();
        for (ListIterator<SubTile> i = mTiles.listIterator(); i.hasNext();) {
            SubTile tile = i.next();

            if (tile.key == null) {
                continue;
            }

            RectF tileBounds = tile.getBounds(context, new FloatSize(tile.getSize()));
            Rect tilespaceTileBounds =
                RectUtils.round(RectUtils.scale(tileBounds, scaleFactor));
            tilespaceTileBounds.offset(-origin.x, -origin.y);

            
            
            if ((!opRegion.op(tilespaceTileBounds, mValidRegion, Region.Op.INTERSECT) &&
                 !Rect.intersects(tilespaceViewport, tilespaceTileBounds)) ||
                (!FloatUtils.fuzzyEquals(tile.getResolution(), getResolution()) &&
                 opRegion.op(tilespaceTileBounds, updateRegion, Region.Op.INTERSECT))) {
                tile.invalidateTexture();

                
                invalidTiles.add(tile);
                i.remove();

                
                mPositionHash.remove(tile.key);
                tile.key = null;
            }
        }

        
        mTiles.addAll(0, invalidTiles);

        
        
        for (int y = origin.y; y <= origin.y + mBufferSize.height; y += mTileSize.height) {
            for (int x = origin.x; x <= origin.x + mBufferSize.width; x += mTileSize.width) {
                
                Rect tilespaceTileRect = new Rect(x - origin.x, y - origin.y,
                                                  (x - origin.x) + mTileSize.width,
                                                  (y - origin.y) + mTileSize.height);
                if (!opRegion.op(tilespaceTileRect, updateRegion, Region.Op.INTERSECT)) {
                    continue;
                }

                
                boolean reusedTile;
                Point tileOrigin = new Point(x, y);
                SubTile tile = mPositionHash.get(longFromPoint(tileOrigin));

                
                if (tile == null) {
                    tile = mTiles.removeFirst();
                    reusedTile = false;
                } else {
                    mTiles.remove(tile);

                    
                    
                    reusedTile = FloatUtils.fuzzyEquals(tile.getResolution(), getResolution());
                }

                
                mTiles.add(tile);

                
                if (opRegion.isComplex()) {
                    Log.w(LOGTAG, "MultiTileLayer encountered complex dirty region");
                }
                Rect dirtyRect = opRegion.getBounds();
                dirtyRect.offset(origin.x - x, origin.y - y);

                
                tile.x = (x - origin.x) / mTileSize.width;
                tile.y = (y - origin.y) / mTileSize.height;
                updateTile(gl, context, tile, tileOrigin, dirtyRect, reusedTile);

                
                
                if (!updateVisible) {
                    mDirtyRegion.op(opRegion, Region.Op.XOR);
                    return mDirtyRegion.isEmpty();
                }
            }
        }

        
        mDirtyRegion.op(updateRegion, Region.Op.XOR);

        return mDirtyRegion.isEmpty();
    }

    @Override
    public void beginTransaction(LayerView aView) {
        super.beginTransaction(aView);

        for (SubTile layer : mTiles) {
            layer.beginTransaction(aView);
        }
    }

    @Override
    public void endTransaction() {
        for (SubTile layer : mTiles) {
            layer.endTransaction();
        }

        super.endTransaction();
    }

    @Override
    public void draw(RenderContext context) {
        for (SubTile layer : mTiles) {
            
            if (layer.key == null) {
                continue;
            }

            
            RectF layerBounds = layer.getBounds(context, new FloatSize(layer.getSize()));
            if (RectF.intersects(layerBounds, context.viewport)) {
                layer.draw(context);
            }
        }
    }

    @Override
    public void setOrigin(Point origin) {
        if (mOrigin == null || !origin.equals(mOrigin)) {
            mOrigin = origin;
            super.setOrigin(origin);
            invalidateBuffer();
        }
    }

    @Override
    public void setResolution(float resolution) {
        if (!FloatUtils.fuzzyEquals(resolution, mResolution)) {
            mResolution = resolution;
            super.setResolution(resolution);
            invalidateBuffer();
        }
    }

    public void setRenderOffset(Point offset) {
        mRenderOffset.set(offset.x, offset.y);
    }

    @Override
    public Region getValidRegion(RenderContext context) {
        Region validRegion = new Region();
        for (SubTile tile : mTiles) {
            if (tile.key == null || tile.getValidTextureArea().isEmpty())
                continue;
            validRegion.op(tile.getValidRegion(context), Region.Op.UNION);
        }

        return validRegion;
    }

    



    protected void invalidateTiles() {
        if (!inTransaction()) {
            throw new RuntimeException("invalidateTiles() is only valid inside a transaction");
        }

        for (SubTile tile : mTiles) {
            
            if (tile.key != null) {
                mPositionHash.remove(tile.key);
                tile.key = null;
            }
            tile.invalidateTexture();
        }
    }

    



    private static class SubTile extends SingleTileLayer {
        public int x;
        public int y;

        public Long key;

        public SubTile(SubImage aImage) {
            super(aImage);

            aImage.tile = this;
        }
    }

    




    private static class SubImage extends CairoImage {
        public SubTile tile;

        private IntSize mTileSize;
        private CairoImage mImage;

        public SubImage(CairoImage image, IntSize tileSize) {
            mTileSize = tileSize;
            mImage = image;
        }

        @Override
        public ByteBuffer getBuffer() {
            
            
            
            IntSize bufferSize = mImage.getSize();
            int bpp = CairoUtils.bitsPerPixelForCairoFormat(getFormat()) / 8;
            int index = (tile.y * (bufferSize.width / mTileSize.width + 1)) + tile.x;

            ByteBuffer buffer = mImage.getBuffer().slice();

            try {
                buffer.position(index * mTileSize.getArea() * bpp);
                buffer = buffer.slice();
                buffer.limit(mTileSize.getArea() * bpp);
            } catch (IllegalArgumentException e) {
                Log.e(LOGTAG, "Tile image-data out of bounds! Tile: (" +
                      tile.x + ", " + tile.y + "), image (" + bufferSize + ")");
                return null;
            }

            return buffer;
        }

        @Override
        public IntSize getSize() {
            return mTileSize;
        }

        @Override
        public int getFormat() {
            return mImage.getFormat();
        }
    }
}

