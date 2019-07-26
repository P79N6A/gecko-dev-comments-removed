




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.R;

import android.content.Context;
import android.util.AttributeSet;

class AlignRightLinkPreference extends LinkPreference {

    public AlignRightLinkPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.preference_rightalign_icon);
    }

    public AlignRightLinkPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setLayoutResource(R.layout.preference_rightalign_icon);
    }
}
