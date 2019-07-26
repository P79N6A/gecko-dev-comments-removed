




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ThumbnailHelper;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff.Mode;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;




public class BookmarkThumbnailView extends ImageView {
    private static final String LOGTAG = "GeckoBookmarkThumbnailView";

    
    private static final int COLOR_FILTER = 0x46FFFFFF;

    
    private static final int DEFAULT_COLOR = 0x46ECF0F3;

    
    private final float mStrokeWidth = getResources().getDisplayMetrics().density * 2;

    
    private static Paint sBorderPaint;

    
    static {
        sBorderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        sBorderPaint.setColor(0xFFCFD9E1);
        sBorderPaint.setStyle(Paint.Style.STROKE);
    }

    public BookmarkThumbnailView(Context context) {
        this(context, null);

        
        setWillNotDraw(false);
    }

    public BookmarkThumbnailView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.bookmarkThumbnailViewStyle);
    }

    public BookmarkThumbnailView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    






    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        
        final int width = getMeasuredWidth();
        final int height = (int) (width * ThumbnailHelper.THUMBNAIL_ASPECT_RATIO);
        setMeasuredDimension(width, height);
    }

    


    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (getBackground() == null) {
            sBorderPaint.setStrokeWidth(mStrokeWidth);
            canvas.drawRect(0, 0, getWidth(), getHeight(), sBorderPaint);
        }
    }

    




    @Override
    public void setBackgroundColor(int color) {
        int colorFilter = color == 0 ? DEFAULT_COLOR : color & COLOR_FILTER;
        Drawable drawable = getResources().getDrawable(R.drawable.bookmark_thumbnail_bg);
        drawable.setColorFilter(colorFilter, Mode.SRC_ATOP);
        setBackgroundDrawable(drawable);
    }
}
