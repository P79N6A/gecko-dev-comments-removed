



package org.mozilla.gecko.toolbar;

import android.content.Context;
import android.util.AttributeSet;

public class TabletTabsButton extends ShapedButton {
    public TabletTabsButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);

        final int curve = (int) (height * 1.125f);

        mPath.reset();

        mPath.moveTo(width, 0);
        mPath.cubicTo((width - (curve * 0.75f)), 0,
                      (width - (curve * 0.25f)), height,
                      (width - curve), height);
        mPath.lineTo(0, height);
        mPath.lineTo(0, 0);
    }
}
