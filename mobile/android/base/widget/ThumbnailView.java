




package org.mozilla.gecko.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.Matrix;
import android.util.AttributeSet;
import android.widget.ImageView;




public class ThumbnailView extends ImageView {
    private static final String LOGTAG = "GeckoThumbnailView";
    final private Matrix mMatrix;
    private int mWidthSpec = -1;
    private int mHeightSpec = -1;
    private boolean mLayoutChanged;

    public ThumbnailView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mMatrix = new Matrix();
        mLayoutChanged = true;
    }

    @Override
    public void onDraw(Canvas canvas) {
        Drawable d = getDrawable();
        if (mLayoutChanged) {
            int w1 = d.getIntrinsicWidth();
            int h1 = d.getIntrinsicHeight();
            int w2 = getWidth();
            int h2 = getHeight();
    
            float scale = (w2/h2 < w1/h1) ? (float)h2/h1 : (float)w2/w1;
            mMatrix.setScale(scale, scale);
        }

        int saveCount = canvas.save();
        canvas.concat(mMatrix);
        d.draw(canvas);
        canvas.restoreToCount(saveCount);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        
        
        
        if (widthMeasureSpec != mWidthSpec || heightMeasureSpec != mHeightSpec) {
            mWidthSpec = widthMeasureSpec;
            mHeightSpec = heightMeasureSpec;
            mLayoutChanged = true;
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
