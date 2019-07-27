



package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.util.AttributeSet;

abstract class NavButton extends ShapedButton {
    protected final Path mBorderPath;
    protected final Paint mBorderPaint;
    protected final float mBorderWidth;

    public NavButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        final Resources res = getResources();
        mBorderWidth = res.getDimension(R.dimen.nav_button_border_width);

        
        mBorderPaint = new Paint();
        mBorderPaint.setAntiAlias(true);
        mBorderPaint.setStrokeWidth(mBorderWidth);
        mBorderPaint.setStyle(Paint.Style.STROKE);

        
        mBorderPath = new Path();

        setPrivateMode(false);
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
