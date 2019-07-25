






































package org.mozilla.gecko.gfx;

import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.util.Log;
import java.nio.FloatBuffer;
import java.util.concurrent.locks.ReentrantLock;
import org.mozilla.gecko.FloatUtils;

public abstract class Layer {
    private final ReentrantLock mTransactionLock;
    private boolean mInTransaction;
    private Rect mNewPosition;
    private float mNewResolution;

    protected Rect mPosition;
    protected float mResolution;

    public Layer() {
        this(null);
    }

    public Layer(IntSize size) {
        mTransactionLock = new ReentrantLock();
        if (size == null) {
            mPosition = new Rect(0, 0, 0, 0);
        } else {
            mPosition = new Rect(0, 0, size.width, size.height);
        }
        mResolution = 1.0f;
    }

    



    public final boolean update(RenderContext context) {
        if (mTransactionLock.isHeldByCurrentThread()) {
            throw new RuntimeException("draw() called while transaction lock held by this " +
                                       "thread?!");
        }

        if (mTransactionLock.tryLock()) {
            try {
                return performUpdates(context);
            } finally {
                mTransactionLock.unlock();
            }
        }

        return false;
    }

    
    public abstract void draw(RenderContext context);

    
    protected RectF getBounds(RenderContext context) {
        return RectUtils.scale(new RectF(mPosition), context.zoomFactor / mResolution);
    }

    




    public Region getValidRegion(RenderContext context) {
        return new Region(RectUtils.round(getBounds(context)));
    }

    






    public void beginTransaction() {
        if (mTransactionLock.isHeldByCurrentThread())
            throw new RuntimeException("Nested transactions are not supported");
        mTransactionLock.lock();
        mInTransaction = true;
        mNewResolution = mResolution;
    }

    
    public void endTransaction() {
        if (!mInTransaction)
            throw new RuntimeException("endTransaction() called outside a transaction");
        mInTransaction = false;
        mTransactionLock.unlock();
    }

    
    protected boolean inTransaction() {
        return mInTransaction;
    }

    
    public Rect getPosition() {
        return mPosition;
    }

    
    public void setPosition(Rect newPosition) {
        if (!mInTransaction)
            throw new RuntimeException("setPosition() is only valid inside a transaction");
        mNewPosition = newPosition;
    }

    
    public float getResolution() {
        return mResolution;
    }

    




    public void setResolution(float newResolution) {
        if (!mInTransaction)
            throw new RuntimeException("setResolution() is only valid inside a transaction");
        mNewResolution = newResolution;
    }

    





    protected boolean performUpdates(RenderContext context) {
        if (mNewPosition != null) {
            mPosition = mNewPosition;
            mNewPosition = null;
        }
        if (mNewResolution != 0.0f) {
            mResolution = mNewResolution;
            mNewResolution = 0.0f;
        }

        return true;
    }

    public static class RenderContext {
        public final RectF viewport;
        public final FloatSize pageSize;
        public final float zoomFactor;
        public final int positionHandle;
        public final int textureHandle;
        public final FloatBuffer coordBuffer;

        public RenderContext(RectF aViewport, FloatSize aPageSize, float aZoomFactor,
                             int aPositionHandle, int aTextureHandle, FloatBuffer aCoordBuffer) {
            viewport = aViewport;
            pageSize = aPageSize;
            zoomFactor = aZoomFactor;
            positionHandle = aPositionHandle;
            textureHandle = aTextureHandle;
            coordBuffer = aCoordBuffer;
        }

        public boolean fuzzyEquals(RenderContext other) {
            if (other == null) {
                return false;
            }
            return RectUtils.fuzzyEquals(viewport, other.viewport)
                && pageSize.fuzzyEquals(other.pageSize)
                && FloatUtils.fuzzyEquals(zoomFactor, other.zoomFactor);
        }
    }
}

