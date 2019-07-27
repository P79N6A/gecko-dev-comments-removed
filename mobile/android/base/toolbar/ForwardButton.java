



package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.util.AttributeSet;

public class ForwardButton extends ShapedButton { 
    private Path mBorderPath;
    private Paint mBorderPaint;
    private final float mBorderWidth;

    public ForwardButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        mBorderWidth = getResources().getDimension(R.dimen.nav_button_border_width);

        
        mBorderPaint = new Paint();
        mBorderPaint.setAntiAlias(true);
        mBorderPaint.setStrokeWidth(mBorderWidth);
        mBorderPaint.setStyle(Paint.Style.STROKE);

        mBorderPath = new Path();

        setPrivateMode(false);
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

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        
        canvas.drawPath(mBorderPath, mBorderPaint);
    }

    
    @Override
    public void onLightweightThemeChanged() {
        final Drawable drawable = mTheme.getDrawable(this);
        if (drawable == null)
            return;

        final StateListDrawable stateList = new StateListDrawable();
        stateList.addState(PRIVATE_PRESSED_STATE_SET, getColorDrawable(R.color.highlight_nav_pb));
        stateList.addState(PRESSED_ENABLED_STATE_SET, getColorDrawable(R.color.highlight_nav));
        stateList.addState(PRIVATE_FOCUSED_STATE_SET, getColorDrawable(R.color.highlight_nav_focused_pb));
        stateList.addState(FOCUSED_STATE_SET, getColorDrawable(R.color.highlight_nav_focused));
        stateList.addState(PRIVATE_STATE_SET, getColorDrawable(R.color.background_private));
        stateList.addState(EMPTY_STATE_SET, drawable);
        
        setBackgroundDrawable(stateList);
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundResource(R.drawable.url_bar_nav_button);
    }
}
