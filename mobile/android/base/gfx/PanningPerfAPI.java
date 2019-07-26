




package org.mozilla.gecko.gfx;

import android.os.SystemClock;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class PanningPerfAPI {
    private static final String LOGTAG = "GeckoPanningPerfAPI";

    
    
    
    private static final int EXPECTED_FRAME_COUNT = 2048;

    private static boolean mRecordingFrames = false;
    private static List<Long> mFrameTimes;
    private static long mFrameStartTime;

    private static boolean mRecordingCheckerboard = false;
    private static List<Float> mCheckerboardAmounts;
    private static long mCheckerboardStartTime;

    private static void initialiseRecordingArrays() {
        if (mFrameTimes == null) {
            mFrameTimes = new ArrayList<Long>(EXPECTED_FRAME_COUNT);
        } else {
            mFrameTimes.clear();
        }
        if (mCheckerboardAmounts == null) {
            mCheckerboardAmounts = new ArrayList<Float>(EXPECTED_FRAME_COUNT);
        } else {
            mCheckerboardAmounts.clear();
        }
    }

    public static void startFrameTimeRecording() {
        if (mRecordingFrames || mRecordingCheckerboard) {
            Log.e(LOGTAG, "Error: startFrameTimeRecording() called while already recording!");
            return;
        }
        mRecordingFrames = true;
        initialiseRecordingArrays();
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
        if (mRecordingCheckerboard || mRecordingFrames) {
            Log.e(LOGTAG, "Error: startCheckerboardRecording() called while already recording!");
            return;
        }
        mRecordingCheckerboard = true;
        initialiseRecordingArrays();
        mCheckerboardStartTime = SystemClock.uptimeMillis();
    }

    public static List<Float> stopCheckerboardRecording() {
        if (!mRecordingCheckerboard) {
            Log.e(LOGTAG, "Error: stopCheckerboardRecording() called when not recording!");
            return null;
        }
        if (mCheckerboardAmounts.size() != mFrameTimes.size()) {
            Log.e(LOGTAG, "Error: Inconsistent number of checkerboard and frame time recordings!");
            return null;
        }
        mRecordingCheckerboard = false;

        
        
        
        long lastTime = 0;
        float totalTime = mFrameTimes.get(mFrameTimes.size() - 1);
        for (int i = 0; i < mCheckerboardAmounts.size(); i++) {
          long elapsedTime = mFrameTimes.get(i) - lastTime;
          mCheckerboardAmounts.set(i, mCheckerboardAmounts.get(i) * elapsedTime / totalTime);
          lastTime = mFrameTimes.get(i);
        }

        return mCheckerboardAmounts;
    }

    public static void recordCheckerboard(float amount) {
        
        if (mRecordingCheckerboard) {
            mFrameTimes.add(SystemClock.uptimeMillis() - mCheckerboardStartTime);
            mCheckerboardAmounts.add(amount);
        }
    }
}
