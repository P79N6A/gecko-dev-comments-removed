




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ThumbnailHelper;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PorterDuff.Mode;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;




public class TopSitesThumbnailView extends ImageView {
    private static final String LOGTAG = "GeckoTopSitesThumbnailView";

    
    private static final int COLOR_FILTER = 0x46FFFFFF;

    
    private final int mDefaultColor = getResources().getColor(R.color.top_site_default);

    
    private final float mStrokeWidth = getResources().getDisplayMetrics().density * 2;

    
    private final Paint mBorderPaint;

    private boolean mResize = false;
    private int mWidth;
    private int mHeight;

    public TopSitesThumbnailView(Context context) {
        this(context, null);

        
        setWillNotDraw(false);

    }

    public TopSitesThumbnailView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.topSitesThumbnailViewStyle);
    }

    public TopSitesThumbnailView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        
        final Resources res = getResources();
        mBorderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBorderPaint.setColor(res.getColor(R.color.top_site_border));
        mBorderPaint.setStyle(Paint.Style.STROKE);
    }

    public void setImageBitmap(Bitmap bm, boolean resize) {
        super.setImageBitmap(bm);
        mResize = resize;
    }

    @Override
    public void setImageResource(int resId) {
        super.setImageResource(resId);
        mResize = false;
    }

    @Override
    public void setImageDrawable(Drawable drawable) {
        super.setImageDrawable(drawable);
        mResize = false;
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if (HardwareUtils.isTablet() && mResize) {
            setScaleType(ScaleType.MATRIX);
            RectF rect = new RectF(0, 0, mWidth, mHeight);
            Matrix matrix = new Matrix();
            matrix.setRectToRect(rect, rect, Matrix.ScaleToFit.CENTER);
            setImageMatrix(matrix);
        }
    }

    






    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        
        mWidth = getMeasuredWidth();
        mHeight = (int) (mWidth * ThumbnailHelper.THUMBNAIL_ASPECT_RATIO);
        setMeasuredDimension(mWidth, mHeight);
    }

    


    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (getBackground() == null) {
            mBorderPaint.setStrokeWidth(mStrokeWidth);
            canvas.drawRect(0, 0, getWidth(), getHeight(), mBorderPaint);
        }
    }

    




    public void setBackgroundColorWithOpacityFilter(int color) {
        setBackgroundColor(color & COLOR_FILTER);
    }

    




    @Override
    public void setBackgroundColor(int color) {
        if (color == 0) {
            color = mDefaultColor;
        }

        Drawable drawable = getResources().getDrawable(R.drawable.top_sites_thumbnail_bg);
        drawable.setColorFilter(color, Mode.SRC_ATOP);
        setBackgroundDrawable(drawable);
    }
}
