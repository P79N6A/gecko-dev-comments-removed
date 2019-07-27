



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

public class BackButton extends ShapedButton { 
    private Path mPath;
    private Path mBorderPath;
    private Paint mBorderPaint;
    private Paint mBorderPrivatePaint;

    public BackButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        mBorderPaint = new Paint();
        mBorderPaint.setAntiAlias(true);
        mBorderPaint.setColor(0xFFB5B5B5);
        mBorderPaint.setStyle(Paint.Style.STROKE);

        mBorderPrivatePaint = new Paint(mBorderPaint);
        mBorderPrivatePaint.setColor(0xFF363B40);

        
        mPath = new Path();
        mBorderPath = new Path();
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);

        mPath.reset();
        mPath.addCircle(width/2, height/2, width/2, Path.Direction.CW);

        float borderWidth = getContext().getResources().getDimension(R.dimen.nav_button_border_width);
        mBorderPaint.setStrokeWidth(borderWidth);
        mBorderPrivatePaint.setStrokeWidth(borderWidth);

        mBorderPath.reset();
        mBorderPath.addCircle(width/2, height/2, (width/2) - borderWidth, Path.Direction.CW);
    }

    @Override
    public void draw(Canvas canvas) {
        mCanvasDelegate.draw(canvas, mPath, getWidth(), getHeight());

        
        canvas.drawPath(mBorderPath, isPrivateMode() ? mBorderPrivatePaint : mBorderPaint);
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
