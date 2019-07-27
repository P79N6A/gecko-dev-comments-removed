




package org.mozilla.gecko.gfx;

import android.os.SystemClock;

















final class DrawTimingQueue {
    private static final String LOGTAG = "GeckoDrawTimingQueue";
    private static final int BUFFER_SIZE = 16;

    private final DisplayPortMetrics[] mMetrics;
    private final long[] mTimestamps;

    private int mHead;
    private int mTail;

    DrawTimingQueue() {
        mMetrics = new DisplayPortMetrics[BUFFER_SIZE];
        mTimestamps = new long[BUFFER_SIZE];
        mHead = BUFFER_SIZE - 1;
    }

    



    boolean add(DisplayPortMetrics metrics) {
        if (mHead == mTail) {
            return false;
        }
        mMetrics[mTail] = metrics;
        mTimestamps[mTail] = SystemClock.uptimeMillis();
        mTail = (mTail + 1) % BUFFER_SIZE;
        return true;
    }

    







    long findTimeFor(DisplayPortMetrics metrics) {
        
        
        
        
        int tail = mTail;
        
        
        int i = (mHead + 1) % BUFFER_SIZE;
        while (i != tail) {
            if (mMetrics[i].fuzzyEquals(metrics)) {
                
                
                long timestamp = mTimestamps[i];
                mHead = i;
                return timestamp;
            }
            i = (i + 1) % BUFFER_SIZE;
        }
        return -1;
    }

    



    void reset() {
        
        mHead = (mTail + BUFFER_SIZE - 1) % BUFFER_SIZE;
    }
}
