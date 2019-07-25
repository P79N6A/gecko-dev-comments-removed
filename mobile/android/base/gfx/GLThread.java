




































package org.mozilla.gecko.gfx;

import android.opengl.GLSurfaceView;
import android.view.SurfaceHolder;
import javax.microedition.khronos.opengles.GL10;
import java.util.concurrent.Future;
import java.util.concurrent.LinkedBlockingQueue;




class GLThread extends Thread {
    private LinkedBlockingQueue<Runnable> mQueue;
    private GLController mController;
    private boolean mRenderQueued;

    public GLThread(GLController controller) {
        mQueue = new LinkedBlockingQueue<Runnable>();
        mController = controller;
    }

    @Override
    public void run() {
        while (true) {
            Runnable runnable;
            try {
                runnable = mQueue.take();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }

            runnable.run();
            if (runnable instanceof ShutdownMessage) {
                break;
            }
        }
    }

    public void recreateSurface() {
        mQueue.add(new RecreateSurfaceMessage());
    }

    public void renderFrame() {
        
        synchronized (this) {
            if (!mRenderQueued) {
                mQueue.add(new RenderFrameMessage());
                mRenderQueued = true;
            }
        }
    }

    public void shutdown() {
        mQueue.add(new ShutdownMessage());
    }

    public void surfaceChanged(int width, int height) {
        mQueue.add(new SizeChangedMessage(width, height));
    }

    public void surfaceCreated() {
        mQueue.add(new SurfaceCreatedMessage());
    }

    public void surfaceDestroyed() {
        mQueue.add(new SurfaceDestroyedMessage());
    }

    private void doRecreateSurface() {
        mController.disposeGLContext();
        mController.initGLContext();
    }

    private GLSurfaceView.Renderer getRenderer() {
        return mController.getView().getRenderer();
    }

    private class RecreateSurfaceMessage implements Runnable {
        public void run() {
            doRecreateSurface();
        }
    }

    private class RenderFrameMessage implements Runnable {
        public void run() {
            synchronized (GLThread.this) {
                mRenderQueued = false;
            }

            
            if (mController.getEGLSurface() == null) {
                return;
            }

            GLSurfaceView.Renderer renderer = getRenderer();
            if (renderer != null) {
                renderer.onDrawFrame((GL10)mController.getGL());
            }

            mController.swapBuffers();
            
            
            
        }
    }

    private class ShutdownMessage implements Runnable {
        public void run() {
            mController.disposeGLContext();
            mController = null;
        }
    }

    private class SizeChangedMessage implements Runnable {
        private int mWidth, mHeight;

        public SizeChangedMessage(int width, int height) {
            mWidth = width;
            mHeight = height;
        }

        public void run() {
            GLSurfaceView.Renderer renderer = getRenderer();
            if (renderer != null) {
                renderer.onSurfaceChanged((GL10)mController.getGL(), mWidth, mHeight);
            }
        }
    }

    private class SurfaceCreatedMessage implements Runnable {
        public void run() {
            if (!mController.hasSurface()) {
                mController.initGLContext();
            }
        }
    }

    private class SurfaceDestroyedMessage implements Runnable {
        public void run() {
            mController.disposeGLContext();
        }
    }
}

