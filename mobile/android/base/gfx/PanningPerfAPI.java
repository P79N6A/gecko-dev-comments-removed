





































package org.mozilla.gecko.gfx;

import java.util.ArrayList;
import java.util.List;
import android.os.SystemClock;
import android.util.Log;

public class PanningPerfAPI {
    private static final String LOGTAG = "GeckoPanningPerfAPI";

    
    
    
    private static final int EXPECTED_FRAME_COUNT = 2048;

    private static boolean mRecordingFrames = false;
    private static List<Long> mFrameTimes;
    private static long mFrameStartTime;

    private static boolean mRecordingCheckerboard = false;
    private static List<Float> mCheckerboardAmounts;
    private static long mCheckerboardStartTime;

    public static void startFrameTimeRecording() {
        if (mRecordingFrames) {
            Log.e(LOGTAG, "Error: startFrameTimeRecording() called while already recording!");
            return;
        }
        mRecordingFrames = true;
        if (mFrameTimes == null) {
            mFrameTimes = new ArrayList<Long>(EXPECTED_FRAME_COUNT);
        } else {
            mFrameTimes.clear();
        }
        mFrameStartTime = SystemClock.uptimeMillis();
    }

    public static List<Long> stopFrameTimeRecording() {
        if (!mRecordingFrames) {
            Log.e(LOGTAG, "Error: stopFrameTimeRecording() called when not recording!");
            return null;
        }
        mRecordingFrames = false;
        return mFrameTimes;
    }

    public static void recordFrameTime() {
        
        if (mRecordingFrames) {
            mFrameTimes.add(SystemClock.uptimeMillis() - mFrameStartTime);
        }
    }

    public static boolean isRecordingCheckerboard() {
        return mRecordingCheckerboard;
    }

    public static void startCheckerboardRecording() {
        if (mRecordingCheckerboard) {
            Log.e(LOGTAG, "Error: startCheckerboardRecording() called while already recording!");
            return;
        }
        mRecordingCheckerboard = true;
        if (mCheckerboardAmounts == null) {
            mCheckerboardAmounts = new ArrayList<Float>(EXPECTED_FRAME_COUNT);
        } else {
            mCheckerboardAmounts.clear();
        }
        mCheckerboardStartTime = SystemClock.uptimeMillis();
    }

    public static List<Float> stopCheckerboardRecording() {
        if (!mRecordingCheckerboard) {
            Log.e(LOGTAG, "Error: stopCheckerboardRecording() called when not recording!");
            return null;
        }
        mRecordingCheckerboard = false;
        return mCheckerboardAmounts;
    }

    public static void recordCheckerboard(float amount) {
        
        if (mRecordingCheckerboard) {
            mCheckerboardAmounts.add(amount);
        }
    }
}
