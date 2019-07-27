




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.mozglue.RobocopTarget;

import android.os.SystemClock;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class PanningPerfAPI {
    private static final String LOGTAG = "GeckoPanningPerfAPI";

    
    
    
    private static final int EXPECTED_FRAME_COUNT = 2048;

    private static boolean mRecordingFrames;
    private static List<Long> mFrameTimes;
    private static long mFrameStartTime;

    private static boolean mRecordingCheckerboard;
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

    @RobocopTarget
    public static void startFrameTimeRecording() {
        if (mRecordingFrames || mRecordingCheckerboard) {
            Log.e(LOGTAG, "Error: startFrameTimeRecording() called while already recording!");
            return;
        }
        mRecordingFrames = true;
        initialiseRecordingArrays();
        mFrameStartTime = SystemClock.uptimeMillis();
    }

    @RobocopTarget
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

    @RobocopTarget
    public static void startCheckerboardRecording() {
        if (mRecordingCheckerboard || mRecordingFrames) {
            Log.e(LOGTAG, "Error: startCheckerboardRecording() called while already recording!");
            return;
        }
        mRecordingCheckerboard = true;
        initialiseRecordingArrays();
        mCheckerboardStartTime = SystemClock.uptimeMillis();
    }

    @RobocopTarget
    public static List<Float> stopCheckerboardRecording() {
        if (!mRecordingCheckerboard) {
            Log.e(LOGTAG, "Error: stopCheckerboardRecording() called when not recording!");
            return null;
        }
        mRecordingCheckerboard = false;

        
        
        
        
        int values = mCheckerboardAmounts.size();
        if (values == 0) {
            Log.w(LOGTAG, "stopCheckerboardRecording() found no checkerboard amounts!");
            return mCheckerboardAmounts;
        }

        
        
        
        long lastTime = 0;
        float totalTime = mFrameTimes.get(values - 1);
        for (int i = 0; i < values; i++) {
            long elapsedTime = mFrameTimes.get(i) - lastTime;
            mCheckerboardAmounts.set(i, mCheckerboardAmounts.get(i) * elapsedTime / totalTime);
            lastTime += elapsedTime;
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
