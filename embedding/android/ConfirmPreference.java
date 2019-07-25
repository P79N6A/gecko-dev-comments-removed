




































package org.mozilla.gecko;
import android.preference.*;
import android.content.*;
import android.util.*;
import android.provider.Browser;

class ConfirmPreference extends DialogPreference {
    String mAction = null;
    Context mContext = null;
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
		    }});
	}
	Log.i("GeckoPref", "action: " + mAction);
    }
}
