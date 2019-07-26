



package org.mozilla.gecko;

import org.mozilla.gecko.util.ActivityResultHandler;

import android.app.Activity;
import android.content.Intent;
import android.os.Environment;
import android.text.format.Time;
import android.util.Log;

import java.io.File;
import java.util.Queue;

class CameraImageResultHandler implements ActivityResultHandler {
    private static final String LOGTAG = "GeckoCameraImageResultHandler";

    private final Queue<String> mFilePickerResult;
    private final ActivityHandlerHelper.FileResultHandler mHandler;

    CameraImageResultHandler(Queue<String> resultQueue) {
        mFilePickerResult = resultQueue;
        mHandler = null;
    }

    
    public CameraImageResultHandler(ActivityHandlerHelper.FileResultHandler handler) {
        mHandler = handler;
        mFilePickerResult = null;
    }

    @Override
    public void onActivityResult(int resultCode, Intent data) {
        if (resultCode != Activity.RESULT_OK) {
            if (mFilePickerResult != null) {
                mFilePickerResult.offer("");
            }
            return;
        }

        File file = new File(Environment.getExternalStorageDirectory(), sImageName);
        sImageName = "";

        if (mFilePickerResult != null) {
            mFilePickerResult.offer(file.getAbsolutePath());
        }

        if (mHandler != null) {
            mHandler.gotFile(file.getAbsolutePath());
        }
    }

    
    

    private static String sImageName = "";

    static String generateImageName() {
        Time now = new Time();
        now.setToNow();
        sImageName = now.format("%Y-%m-%d %H.%M.%S") + ".jpg";
        return sImageName;
    }
}
