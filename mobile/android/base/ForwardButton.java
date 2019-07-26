



package org.mozilla.gecko;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff.Mode;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.util.AttributeSet;

public class ForwardButton extends ShapedButton { 
    private Path mBorderPath;
    private Paint mBorderPaint;

    public ForwardButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        
        mBorderPaint = new Paint();
        mBorderPaint.setAntiAlias(true);
        mBorderPaint.setColor(0xFF000000);
        mBorderPaint.setStyle(Paint.Style.STROKE);

        mBorderPath = new Path();
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);

        float borderWidth = getContext().getResources().getDimension(R.dimen.nav_button_border_width);
        mBorderPaint.setStrokeWidth(borderWidth);

        mBorderPath.reset();
        mBorderPath.moveTo(width - borderWidth, 0);
        mBorderPath.lineTo(width - borderWidth, height);

        mBorderPaint.setShader(new LinearGradient(0, 0, 
                                                  0, height, 
                                                  0xFF898D8F, 0xFFFEFEFE,
                                                  Shader.TileMode.CLAMP));
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        
        canvas.drawPath(mBorderPath, mBorderPaint);
    }

    
    @Override
    public void onLightweightThemeChanged() {
        Drawable drawable = mActivity.getLightweightTheme().getDrawable(this);
        if (drawable == null)
            return;

        Resources resources = getContext().getResources();
        StateListDrawable stateList = new StateListDrawable();

        stateList.addState(new int[] { android.R.attr.state_pressed }, resources.getDrawable(R.drawable.highlight));
        stateList.addState(new int[] {}, drawable);

        setBackgroundDrawable(stateList);
    }

    @Override
    public void onLightweightThemeReset() {
        setBackgroundResource(R.drawable.address_bar_nav_button);
    }
}
