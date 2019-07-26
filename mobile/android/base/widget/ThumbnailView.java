




package org.mozilla.gecko.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.Matrix;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageView;




public class ThumbnailView extends ImageView {
    private static final String LOGTAG = "GeckoThumbnailView";
    private Matrix mMatrix = null;
    private int mWidthSpec = -1;
    private int mHeightSpec = -1;

    public ThumbnailView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mMatrix = new Matrix();
    }

    @Override
    public void onDraw(Canvas canvas) {
        Drawable d = getDrawable();
        if (mMatrix == null) {
            int w1 = d.getIntrinsicWidth();
            int h1 = d.getIntrinsicHeight();
            int w2 = getWidth();
            int h2 = getHeight();
    
            float scale = 1.0f;
            if (w2/h2 < w1/h1) {
                scale = (float)h2/h1;
            } else {
                scale = (float)w2/w1;
            }

            mMatrix.reset();
            mMatrix.setScale(scale, scale);
        }

        int saveCount = canvas.save();
        canvas.concat(mMatrix);
        d.draw(canvas);
        canvas.restoreToCount(saveCount);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
        
        
        
        if (widthMeasureSpec != mWidthSpec || heightMeasureSpec != mHeightSpec) {
            mWidthSpec = widthMeasureSpec;
            mHeightSpec = heightMeasureSpec;
            mMatrix = null;
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
