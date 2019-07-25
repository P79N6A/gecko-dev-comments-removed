





































package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.util.Log;
import java.util.concurrent.locks.ReentrantLock;
import javax.microedition.khronos.opengles.GL10;

public abstract class Layer {
    private final ReentrantLock mTransactionLock;
    private boolean mInTransaction;
    private Point mOrigin;
    private Point mNewOrigin;
    private float mResolution;
    private float mNewResolution;
    private LayerView mView;

    public Layer() {
        mTransactionLock = new ReentrantLock();
        mOrigin = new Point(0, 0);
        mResolution = 1.0f;
    }

    
    public final void update(GL10 gl) {
        if (mTransactionLock.isHeldByCurrentThread()) {
            throw new RuntimeException("draw() called while transaction lock held by this " +
                                       "thread?!");
        }

        if (mTransactionLock.tryLock()) {
            try {
                performUpdates(gl);
            } finally {
                mTransactionLock.unlock();
            }
        }
    }

    
    public final void transform(GL10 gl) {
        gl.glScalef(1.0f / mResolution, 1.0f / mResolution, 1.0f);
        gl.glTranslatef(mOrigin.x, mOrigin.y, 0.0f);
    }

    
    public final void draw(GL10 gl) {
        gl.glPushMatrix();

        onDraw(gl);

        gl.glPopMatrix();
    }

    






    public void beginTransaction(LayerView aView) {
        if (mTransactionLock.isHeldByCurrentThread())
            throw new RuntimeException("Nested transactions are not supported");
        mTransactionLock.lock();
        mView = aView;
        mInTransaction = true;
        mNewResolution = mResolution;
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

    




    protected abstract void onDraw(GL10 gl);

    




    protected void performUpdates(GL10 gl) {
        if (mNewOrigin != null) {
            mOrigin = mNewOrigin;
            mNewOrigin = null;
        }
        mResolution = mNewResolution;
    }
}

