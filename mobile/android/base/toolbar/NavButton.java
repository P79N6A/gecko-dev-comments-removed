



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

    protected final int mBorderColor;
    protected final int mBorderColorPrivate;

    public NavButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        final Resources res = getResources();
        mBorderColor = res.getColor(R.color.nav_button_border_color);
        mBorderColorPrivate = res.getColor(R.color.nav_button_border_color_private);
        mBorderWidth = res.getDimension(R.dimen.nav_button_border_width);

        
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
        mBorderPaint.setColor(isPrivate ? mBorderColorPrivate : mBorderColor);
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        
        canvas.drawPath(mBorderPath, mBorderPaint);
    }

    
    @Override
    public void onLightweightThemeChanged() {
        final Drawable drawable = BrowserToolbar.getLightweightThemeDrawable(this, getResources(),
                getTheme(), R.color.background_normal);

        if (drawable == null) {
            return;
        }

        final StateListDrawable stateList = new StateListDrawable();
        stateList.addState(PRIVATE_PRESSED_STATE_SET, getColorDrawable(R.color.placeholder_active_grey));
        stateList.addState(PRESSED_ENABLED_STATE_SET, getColorDrawable(R.color.new_tablet_highlight));
        stateList.addState(PRIVATE_FOCUSED_STATE_SET, getColorDrawable(R.color.new_tablet_highlight_focused_pb));
        stateList.addState(FOCUSED_STATE_SET, getColorDrawable(R.color.new_tablet_highlight_focused));
        stateList.addState(PRIVATE_STATE_SET, getColorDrawable(R.color.private_toolbar_grey));
        stateList.addState(EMPTY_STATE_SET, drawable);

        setBackgroundDrawable(stateList);
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundResource(R.drawable.new_tablet_url_bar_nav_button);
    }
}
