




































package org.mozilla.gecko;

import android.content.Context;
import android.preference.DialogPreference;
import android.provider.Browser;
import android.util.AttributeSet;
import android.util.Log;

class ConfirmPreference extends DialogPreference {
    private String mAction = null;
    private Context mContext = null;
    public ConfirmPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mAction = attrs.getAttributeValue(null, "action");
        mContext = context;
    }
    public ConfirmPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mAction = attrs.getAttributeValue(null, "action");
        mContext = context;
    }
    protected void onDialogClosed(boolean positiveResult) {
        if (!positiveResult)
            return;
        if ("clear_history".equalsIgnoreCase(mAction)) {
            GeckoAppShell.getHandler().post(new Runnable(){
                public void run() {
                    Browser.clearHistory(mContext.getContentResolver());
                }
            });
        } else if ("clear_private_data".equalsIgnoreCase(mAction)) {
            GeckoAppShell.getHandler().post(new Runnable(){
                public void run() {
                    GeckoAppShell.sendEventToGecko(new GeckoEvent("Sanitize:ClearAll", null));
                }
            });
        }
        Log.i("GeckoPref", "action: " + mAction);
    }
}
