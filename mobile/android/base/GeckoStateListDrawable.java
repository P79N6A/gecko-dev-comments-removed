




































package org.mozilla.gecko;

import android.graphics.Color;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.StateListDrawable;
import android.graphics.LightingColorFilter;

public class GeckoStateListDrawable extends StateListDrawable {
    private LightingColorFilter mFilter;

    public void initializeFilter(int color) {
        mFilter = new LightingColorFilter(Color.WHITE, color);
    }

    @Override
    protected boolean onStateChange(int[] stateSet) {
        for (int state: stateSet) {
            if (state == android.R.attr.state_pressed || state == android.R.attr.state_focused) {
                super.onStateChange(stateSet);
                ((LayerDrawable) getCurrent()).getDrawable(0).setColorFilter(mFilter);
                return true;
            }
        }

        return super.onStateChange(stateSet);
    }
}
