



package org.mozilla.gecko;

import android.content.Intent;
import android.util.Log;

import java.util.Queue;

class FilePickerResultHandlerSync extends FilePickerResultHandler {
    private static final String LOGTAG = "GeckoFilePickerResultHandlerSync";

    FilePickerResultHandlerSync(Queue<String> resultQueue) {
        super(resultQueue, null);
    }

    
    public FilePickerResultHandlerSync(ActivityHandlerHelper.FileResultHandler handler) {
        super(null, handler);
    }

    @Override
    public void onActivityResult(int resultCode, Intent data) {
        if (mFilePickerResult != null)
            mFilePickerResult.offer(handleActivityResult(resultCode, data));

        if (mHandler != null)
            mHandler.gotFile(handleActivityResult(resultCode, data));
    }
}
