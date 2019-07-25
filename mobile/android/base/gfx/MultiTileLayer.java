




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.SingleTileLayer;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.util.Log;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import javax.microedition.khronos.opengles.GL10;






public class MultiTileLayer extends Layer {
    private static final String LOGTAG = "GeckoMultiTileLayer";

    private final CairoImage mImage;
    private IntSize mTileSize;
    private IntSize mBufferSize;
    private final ArrayList<SubTile> mTiles;

    public MultiTileLayer(CairoImage image, IntSize tileSize) {
        super();

        mImage = image;
        mTileSize = tileSize;
        mBufferSize = new IntSize(0, 0);
        mTiles = new ArrayList<SubTile>();
    }

    public void invalidate(Rect dirtyRect) {
        if (!inTransaction()) {
            throw new RuntimeException("invalidate() is only valid inside a transaction");
        }

        for (SubTile layer : mTiles) {
            IntSize tileSize = layer.getSize();
            Rect tileRect = new Rect(layer.x, layer.y, layer.x + tileSize.width, layer.y + tileSize.height);

            if (tileRect.intersect(dirtyRect)) {
                tileRect.offset(-layer.x, -layer.y);
                layer.invalidate(tileRect);
            }
        }
    }

    public void invalidate() {
        for (SubTile layer : mTiles) {
            layer.invalidate();
        }
    }

    @Override
    public IntSize getSize() {
        return mImage.getSize();
    }

    private void validateTiles() {
        IntSize size = getSize();

        if (size.equals(mBufferSize)) {
            return;
        }

        
        mTiles.clear();
        int offset = 0;
        final int format = mImage.getFormat();
        final ByteBuffer buffer = mImage.getBuffer().slice();
        final int bpp = CairoUtils.bitsPerPixelForCairoFormat(format) / 8;
        for (int y = 0; y < size.height; y += mTileSize.height) {
            for (int x = 0; x < size.width; x += mTileSize.width) {
                
                
                
                final IntSize layerSize =
                    new IntSize(Math.min(mTileSize.width, size.width - x),
                                Math.min(mTileSize.height, size.height - y));
                final int tileOffset = offset;

                CairoImage subImage = new CairoImage() {
                    @Override
                    public ByteBuffer getBuffer() {
                        
                        
                        
                        buffer.position(tileOffset);
                        ByteBuffer tileBuffer = buffer.slice();
                        tileBuffer.limit(layerSize.getArea() * bpp);

                        return tileBuffer;
                    }

                    @Override
                    public IntSize getSize() {
                        return layerSize;
                    }

                    @Override
                    public int getFormat() {
                        return format;
                    }
                };

                mTiles.add(new SubTile(subImage, x, y));
                offset += layerSize.getArea() * bpp;
            }
        }

        
        refreshTileMetrics(getOrigin(), getResolution(), false);

        mBufferSize = size;
    }

    @Override
    protected boolean performUpdates(GL10 gl, RenderContext context) {
        super.performUpdates(gl, context);

        validateTiles();

        
        int dirtyTiles = 0;
        boolean screenUpdateDone = false;
        SubTile firstDirtyTile = null;
        for (SubTile layer : mTiles) {
            
            
            boolean invalid = layer.getSkipTextureUpdate();
            layer.setSkipTextureUpdate(true);
            layer.performUpdates(gl, context);

            RectF layerBounds = layer.getBounds(context, new FloatSize(layer.getSize()));
            boolean isDirty = layer.isDirty();

            if (isDirty) {
                if (!RectF.intersects(layerBounds, context.viewport)) {
                    if (firstDirtyTile == null)
                        firstDirtyTile = layer;
                    dirtyTiles ++;
                    invalid = true;
                } else {
                    
                    
                    layer.setSkipTextureUpdate(false);
                    screenUpdateDone = true;
                    layer.performUpdates(gl, context);
                    invalid = false;
                }
            }

            
            
            
            
            layer.setSkipTextureUpdate(invalid);
        }

        
        
        
        
        if (!screenUpdateDone && firstDirtyTile != null) {
            firstDirtyTile.setSkipTextureUpdate(false);
            firstDirtyTile.performUpdates(gl, context);
            dirtyTiles --;
        }

        return (dirtyTiles == 0);
    }

    private void refreshTileMetrics(Point origin, float resolution, boolean inTransaction) {
        IntSize size = getSize();
        for (SubTile layer : mTiles) {
            if (!inTransaction) {
                layer.beginTransaction(null);
            }

            if (origin != null) {
                layer.setOrigin(new Point(origin.x + layer.x, origin.y + layer.y));
            }
            if (resolution >= 0.0f) {
                layer.setResolution(resolution);
            }

            if (!inTransaction) {
                layer.endTransaction();
            }
        }
    }

    @Override
    public void setOrigin(Point newOrigin) {
        super.setOrigin(newOrigin);
        refreshTileMetrics(newOrigin, -1, true);
    }

    @Override
    public void setResolution(float newResolution) {
        super.setResolution(newResolution);
        refreshTileMetrics(null, newResolution, true);
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
            
            
            if (layer.getSkipTextureUpdate())
                continue;

            
            RectF layerBounds = layer.getBounds(context, new FloatSize(layer.getSize()));
            if (RectF.intersects(layerBounds, context.viewport))
                layer.draw(context);
        }
    }

    @Override
    public Region getValidRegion(RenderContext context) {
        Region validRegion = new Region();
        for (SubTile tile : mTiles) {
            if (tile.getSkipTextureUpdate())
                continue;
            validRegion.op(tile.getValidRegion(context), Region.Op.UNION);
        }

        return validRegion;
    }

    class SubTile extends SingleTileLayer {
        public int x;
        public int y;

        public SubTile(CairoImage mImage, int mX, int mY) {
            super(mImage);
            x = mX;
            y = mY;
        }
    }
}

