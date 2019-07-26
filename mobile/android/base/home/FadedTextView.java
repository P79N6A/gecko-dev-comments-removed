




package org.mozilla.gecko.home;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.TextView;

import org.mozilla.gecko.R;





public class FadedTextView extends TextView {

    
    private int mFadeWidth;

    
    private int mCompoundPadding;

    public FadedTextView(Context context) {
        this(context, null);
    }

    public FadedTextView(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.textViewStyle);
    }

    public FadedTextView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.FadedTextView);
        mFadeWidth = a.getDimensionPixelSize(R.styleable.FadedTextView_fadeWidth, 0);
        a.recycle();

        mCompoundPadding = getCompoundDrawablePadding();
    }

    


    @Override
    public void onDraw(Canvas canvas) {
        int width = getMeasuredWidth();

        
        
        if (getLayout().getLineWidth(0) > width) {
            final Drawable leftDrawable = getCompoundDrawables()[0];
            int drawableWidth = 0;
            if (leftDrawable != null) {
                drawableWidth = leftDrawable.getIntrinsicWidth() + mCompoundPadding;
                width -= drawableWidth;
            }

            int color = getCurrentTextColor();
            float stop = ((float) (width - mFadeWidth) / (float) width);
            LinearGradient gradient = new LinearGradient(0, 0, width, 0,
                                                         new int[] { color, color, 0x0 },
                                                         new float[] { 0, stop, 1.0f - (drawableWidth / width) },
                                                         Shader.TileMode.CLAMP);
            getPaint().setShader(gradient);
        } else {
            getPaint().setShader(null);
        }

        
        super.onDraw(canvas);
    }
}
