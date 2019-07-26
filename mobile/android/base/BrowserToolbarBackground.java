



package org.mozilla.gecko;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.PorterDuff.Mode;
import android.util.AttributeSet;
import android.widget.LinearLayout;

public class BrowserToolbarBackground extends LinearLayout
                                      implements CanvasDelegate.DrawManager { 
    Path mPath;
    CanvasDelegate mCanvasDelegate;

    public BrowserToolbarBackground(Context context, AttributeSet attrs) {
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
        mPath.moveTo(width, height);
        mPath.cubicTo((width - (curve * 0.75f)), height,
                      (width - (curve * 0.25f)), 0,
                      (width - curve), 0);
        mPath.lineTo(width, 0);
        mPath.lineTo(width, height);
    }

    @Override
    public void draw(Canvas canvas) {
        mCanvasDelegate.draw(canvas, mPath, getWidth(), getHeight());
    }

    @Override
    public void defaultDraw(Canvas canvas) {
        super.draw(canvas);
    }
}
