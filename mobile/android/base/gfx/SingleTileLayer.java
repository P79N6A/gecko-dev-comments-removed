




package org.mozilla.gecko.gfx;

import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.RegionIterator;
import android.opengl.GLES20;

import java.nio.FloatBuffer;






public class SingleTileLayer extends TileLayer {
    private static final String LOGTAG = "GeckoSingleTileLayer";

    private Rect mMask;

    
    
    private final RectF mBounds;
    private final RectF mTextureBounds;
    private final RectF mViewport;
    private final Rect mIntBounds;
    private final Rect mSubRect;
    private final RectF mSubRectF;
    private final Region mMaskedBounds;
    private final Rect mCropRect;
    private final RectF mObjRectF;
    private final float[] mCoords;

    public SingleTileLayer(CairoImage image) {
        this(false, image);
    }

    public SingleTileLayer(boolean repeat, CairoImage image) {
        this(image, repeat ? TileLayer.PaintMode.REPEAT : TileLayer.PaintMode.NORMAL);
    }

    public SingleTileLayer(CairoImage image, TileLayer.PaintMode paintMode) {
        super(image, paintMode);

        mBounds = new RectF();
        mTextureBounds = new RectF();
        mViewport = new RectF();
        mIntBounds = new Rect();
        mSubRect = new Rect();
        mSubRectF = new RectF();
        mMaskedBounds = new Region();
        mCropRect = new Rect();
        mObjRectF = new RectF();
        mCoords = new float[20];
    }

    


    public void setMask(Rect aMaskRect) {
        mMask = aMaskRect;
    }

    @Override
    public void draw(RenderContext context) {
        
        
        if (!initialized())
            return;

        mViewport.set(context.viewport);

        if (repeats()) {
            
            
            
            mBounds.set(getBounds(context));
            mTextureBounds.set(0.0f, 0.0f, mBounds.width(), mBounds.height());
            mBounds.set(0.0f, 0.0f, mViewport.width(), mViewport.height());
        } else if (stretches()) {
            
            
            mBounds.set(context.pageRect);
            mTextureBounds.set(mBounds);
        } else {
            mBounds.set(getBounds(context));
            mTextureBounds.set(mBounds);
        }

        mBounds.roundOut(mIntBounds);
        mMaskedBounds.set(mIntBounds);
        if (mMask != null) {
            mMaskedBounds.op(mMask, Region.Op.DIFFERENCE);
            if (mMaskedBounds.isEmpty())
                return;
        }

        
        
        RegionIterator i = new RegionIterator(mMaskedBounds);
        while (i.next(mSubRect)) {
            
            
            mSubRectF.set(Math.max(mBounds.left, (float)mSubRect.left),
                          Math.max(mBounds.top, (float)mSubRect.top),
                          Math.min(mBounds.right, (float)mSubRect.right),
                          Math.min(mBounds.bottom, (float)mSubRect.bottom));

            
            
            mCropRect.set(Math.round(mSubRectF.left - mBounds.left),
                          Math.round(mBounds.bottom - mSubRectF.top),
                          Math.round(mSubRectF.right - mBounds.left),
                          Math.round(mBounds.bottom - mSubRectF.bottom));

            mObjRectF.set(mSubRectF.left - mViewport.left,
                          mViewport.bottom - mSubRectF.bottom,
                          mSubRectF.right - mViewport.left,
                          mViewport.bottom - mSubRectF.top);

            fillRectCoordBuffer(mCoords, mObjRectF, mViewport.width(), mViewport.height(),
                                mCropRect, mTextureBounds.width(), mTextureBounds.height());

            FloatBuffer coordBuffer = context.coordBuffer;
            int positionHandle = context.positionHandle;
            int textureHandle = context.textureHandle;

            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, getTextureID());

            
            coordBuffer.position(0);
            coordBuffer.put(mCoords);

            
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20, coordBuffer);

            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20, coordBuffer);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        }
    }
}
