



package org.mozilla.gecko.toolbar;

import android.content.Context;
import android.graphics.Path;
import android.util.AttributeSet;

public class BackButton extends NavButton {
    public BackButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);

        mPath.reset();
        mPath.addCircle(width/2, height/2, width/2, Path.Direction.CW);

        mBorderPath.reset();
        mBorderPath.addCircle(width/2, height/2, (width/2) - (mBorderWidth/2), Path.Direction.CW);
    }
}
