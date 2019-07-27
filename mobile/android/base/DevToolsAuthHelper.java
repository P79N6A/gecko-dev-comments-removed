




package org.mozilla.gecko;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.InputOptionsUtils;




public class DevToolsAuthHelper {

    public static void scan(Context context, final EventCallback callback) {
        final Intent intent = InputOptionsUtils.createQRCodeReaderIntent();

        intent.putExtra("PROMPT_MESSAGE", context.getString(R.string.devtools_auth_scan_header));

        Activity activity = GeckoAppShell.getGeckoInterface().getActivity();
        ActivityHandlerHelper.startIntentForActivity(activity, intent, new ActivityResultHandler() {
            @Override
            public void onActivityResult(int resultCode, Intent intent) {
                if (resultCode == Activity.RESULT_OK) {
                    String text = intent.getStringExtra("SCAN_RESULT");
                    callback.sendSuccess(text);
                } else {
                    callback.sendError(resultCode);
                }
            }
        });
    }

}
