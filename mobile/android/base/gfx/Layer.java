





































package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.Log;
import java.util.concurrent.locks.ReentrantLock;
import javax.microedition.khronos.opengles.GL10;
import org.mozilla.gecko.FloatUtils;

public abstract class Layer {
    private final ReentrantLock mTransactionLock;
    private boolean mInTransaction;
    private Point mOrigin;
    private Point mNewOrigin;
    private float mResolution;
    private float mNewResolution;
    private AbstractLayerView mView;

    public Layer() {
        mTransactionLock = new ReentrantLock();
        mOrigin = new Point(0, 0);
        mResolution = 1.0f;
    }

    





    public final boolean update(GL10 gl, RenderContext context) {
        boolean startTransaction = true;
        if (mTransactionLock.isHeldByCurrentThread()) {
            startTransaction = false;
        }

        
        
        if (startTransaction && !mTransactionLock.tryLock()) {
            return false;
        }

        mInTransaction = true;
        try {
            return performUpdates(gl, context);
        } finally {
            if (startTransaction) {
                mInTransaction = false;
                mTransactionLock.unlock();
            }
        }
    }

    
    public abstract void draw(RenderContext context);

    
    public abstract IntSize getSize();

    
    protected RectF getBounds(RenderContext context, FloatSize size) {
        float scaleFactor = context.zoomFactor / mResolution;
        float x = mOrigin.x * scaleFactor, y = mOrigin.y * scaleFactor;
        float width = size.width * scaleFactor, height = size.height * scaleFactor;
        return new RectF(x, y, x + width, y + height);
    }

    






    public void beginTransaction(AbstractLayerView aView) {
        if (mTransactionLock.isHeldByCurrentThread())
            throw new RuntimeException("Nested transactions are not supported");
        mTransactionLock.lock();
        mView = aView;
        mInTransaction = true;
    }

    public void beginTransaction() {
        beginTransaction(null);
    }

    
    public void endTransaction() {
        if (!mInTransaction)
            throw new RuntimeException("endTransaction() called outside a transaction");
        mInTransaction = false;
        mTransactionLock.unlock();

        if (mView != null)
            mView.requestRender();
    }

    
    protected boolean inTransaction() {
        return mInTransaction;
    }

    
    public Point getOrigin() {
        return mOrigin;
    }

    
    public void setOrigin(Point newOrigin) {
        if (!mInTransaction)
            throw new RuntimeException("setOrigin() is only valid inside a transaction");
        mNewOrigin = newOrigin;
    }

    
    public float getResolution() {
        return mResolution;
    }

    




    public void setResolution(float newResolution) {
        if (!mInTransaction)
            throw new RuntimeException("setResolution() is only valid inside a transaction");
        mNewResolution = newResolution;
    }

    





    protected boolean performUpdates(GL10 gl, RenderContext context) {
        if (mNewOrigin != null) {
            mOrigin = mNewOrigin;
            mNewOrigin = null;
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

        public RenderContext(RectF aViewport, FloatSize aPageSize, float aZoomFactor) {
            viewport = aViewport;
            pageSize = aPageSize;
            zoomFactor = aZoomFactor;
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

