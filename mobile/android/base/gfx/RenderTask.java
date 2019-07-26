




package org.mozilla.gecko.gfx;





public abstract class RenderTask {
    


    public final boolean runAfter;

    


    private long mStartTime;

    


    private boolean mResetStartTime = true;

    















    public final boolean run(long timeDelta, long currentFrameStartTime) {
        if (mResetStartTime) {
            mStartTime = currentFrameStartTime;
            mResetStartTime = false;
        }
        return internalRun(timeDelta, currentFrameStartTime);
    }

    





    protected abstract boolean internalRun(long timeDelta, long currentFrameStartTime);

    public RenderTask(boolean aRunAfter) {
        runAfter = aRunAfter;
    }

    




    public long getStartTime() {
        return mStartTime;
    }

    



    public void resetStartTime() {
        mResetStartTime = true;
    }
}
