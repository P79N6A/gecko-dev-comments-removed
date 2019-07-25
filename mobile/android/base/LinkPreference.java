




































package org.mozilla.gecko;

import android.content.Context;
import android.preference.Preference;
import android.util.AttributeSet;

class LinkPreference extends Preference {
    private String mUrl = null;

    public LinkPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mUrl = attrs.getAttributeValue(null, "url");
    }
    public LinkPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mUrl = attrs.getAttributeValue(null, "url");
    }

    @Override
    protected void onClick() {
        GeckoApp.mAppContext.loadUrlInNewTab(mUrl);
        callChangeListener(mUrl);
    }
}
