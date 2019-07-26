



package org.mozilla.gecko;

import android.content.Intent;
import android.util.Log;

import java.util.concurrent.SynchronousQueue;

class FilePickerResultHandlerSync extends FilePickerResultHandler {
    private static final String LOGTAG = "GeckoFilePickerResultHandlerSync";

    FilePickerResultHandlerSync(SynchronousQueue<String> resultQueue) {
        super(resultQueue);
    }

    @Override
    public void onActivityResult(int resultCode, Intent data) {
        try {
            mFilePickerResult.put(handleActivityResult(resultCode, data));
        } catch (InterruptedException e) {
            Log.i(LOGTAG, "error returning file picker result", e);
        }
    }
}
