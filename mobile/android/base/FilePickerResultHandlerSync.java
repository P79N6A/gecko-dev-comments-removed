



package org.mozilla.gecko;

import android.content.Intent;
import android.util.Log;

import java.util.Queue;

class FilePickerResultHandlerSync extends FilePickerResultHandler {
    private static final String LOGTAG = "GeckoFilePickerResultHandlerSync";

    FilePickerResultHandlerSync(Queue<String> resultQueue) {
        super(resultQueue);
    }

    @Override
    public void onActivityResult(int resultCode, Intent data) {
        mFilePickerResult.offer(handleActivityResult(resultCode, data));
    }
}
