




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ThumbnailHelper;

import android.content.Context;
import android.graphics.PorterDuff.Mode;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;




public class BookmarkThumbnailView extends ImageView {
    private static final String LOGTAG = "GeckoBookmarkThumbnailView";

    
    private static final int COLOR_FILTER = 0x46FFFFFF;

    
    private static final int DEFAULT_COLOR = 0x46ECF0F3;

    public BookmarkThumbnailView(Context context) {
        this(context, null);
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
    public void setBackgroundColor(int color) {
        int colorFilter = color == 0 ? DEFAULT_COLOR : color & COLOR_FILTER;
        Drawable drawable = getResources().getDrawable(R.drawable.favicon_bg);
        drawable.setColorFilter(colorFilter, Mode.SRC_ATOP);
        setBackgroundDrawable(drawable);
    }
}
