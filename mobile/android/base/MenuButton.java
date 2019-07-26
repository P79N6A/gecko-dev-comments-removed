



package org.mozilla.gecko;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.PorterDuff.Mode;
import android.util.AttributeSet;

public class MenuButton extends ShapedButton {

    public MenuButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        mPath = new Path();
        mCanvasDelegate = new CanvasDelegate(this, Mode.DST_OUT);
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        int width = getMeasuredWidth();
        int height = getMeasuredHeight();
        float curve = height * 1.125f;

        mPath.reset();

        
        if (mSide == CurveTowards.RIGHT) {
            mPath.moveTo(0, 0);
            mPath.cubicTo(curve * 0.75f, 0,
                          curve * 0.25f, height,
                          curve, height);
            mPath.lineTo(0, height);
            mPath.lineTo(0, 0);
        } else if (mSide == CurveTowards.LEFT) {
            mPath.moveTo(width, 0);
            mPath.cubicTo((width - (curve * 0.75f)), 0,
                          (width - (curve * 0.25f)), height,
                          (width - curve), height);
            mPath.lineTo(width, height);
            mPath.lineTo(width, 0);
        }
    }
}
