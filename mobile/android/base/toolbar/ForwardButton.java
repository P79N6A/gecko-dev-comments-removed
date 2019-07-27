



package org.mozilla.gecko.toolbar;

import android.content.Context;
import android.util.AttributeSet;

public class ForwardButton extends NavButton {
    public ForwardButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    
    @Override
    public void setPrivateMode(boolean isPrivate) {
        super.setPrivateMode(isPrivate);
        mBorderPaint.setColor(isPrivate ? 0xFF363B40 : 0xFFBFBFBF);
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);

        mBorderPath.reset();
        mBorderPath.moveTo(width - mBorderWidth, 0);
        mBorderPath.lineTo(width - mBorderWidth, height);
    }
}
