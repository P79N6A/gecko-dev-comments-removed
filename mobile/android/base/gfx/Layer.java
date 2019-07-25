




































package org.mozilla.gecko.gfx;

import android.graphics.PointF;
import android.util.Log;
import java.util.concurrent.locks.ReentrantLock;
import javax.microedition.khronos.opengles.GL10;

public abstract class Layer {
    private final ReentrantLock mTransactionLock;
    private boolean mInTransaction;
    private PointF mOrigin;
    private PointF mNewOrigin;

    public Layer() {
        mTransactionLock = new ReentrantLock();
        mOrigin = new PointF(0.0f, 0.0f);
    }

    
    public final void draw(GL10 gl) {
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

        gl.glPushMatrix();
        gl.glTranslatef(mOrigin.x, mOrigin.y, 0.0f);

        onDraw(gl);

        gl.glPopMatrix();
    }

    






    public void beginTransaction() {
        if (mTransactionLock.isHeldByCurrentThread())
            throw new RuntimeException("Nested transactions are not supported");
        mTransactionLock.lock();
        mInTransaction = true;
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

    
    public PointF getOrigin() {
        return mOrigin;
    }

    
    public void setOrigin(PointF newOrigin) {
        if (!mInTransaction)
            throw new RuntimeException("setOrigin() is only valid inside a transaction");
        mNewOrigin = newOrigin;
    }

    




    protected abstract void onDraw(GL10 gl);

    




    protected void performUpdates(GL10 gl) {
        if (mNewOrigin != null) {
            mOrigin = mNewOrigin;
            mNewOrigin = null;
        }
    }
}

