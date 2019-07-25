




































package org.mozilla.gecko.gfx;

import java.util.ArrayList;
import java.util.List;
import android.os.SystemClock;
import android.util.Log;

public class PanningPerfAPI {
    private static final String LOGTAG = "GeckoPanningPerfAPI";

    
    
    
    private static final int EXPECTED_FRAME_COUNT = 2048;

    private static boolean mRecording = false;
    private static List<Long> mFrameTimes;
    private static long mStartTime;

    public static void startFrameTimeRecording() {
        if (mRecording) {
            Log.e(LOGTAG, "Error: startFrameTimeRecording() called while already recording!");
            return;
        }
        mRecording = true;
        if (mFrameTimes == null) {
            mFrameTimes = new ArrayList<Long>(EXPECTED_FRAME_COUNT);
        } else {
            mFrameTimes.clear();
        }
        mStartTime = SystemClock.uptimeMillis();
    }

    public static List<Long> stopFrameTimeRecording() {
        if (!mRecording) {
            Log.e(LOGTAG, "Error: stopFrameTimeRecording() called when not recording!");
            return null;
        }
        mRecording = false;
        return mFrameTimes;
    }

    public static void recordFrameTime() {
        
        if (mRecording) {
            mFrameTimes.add(SystemClock.uptimeMillis() - mStartTime);
        }
    }
}
