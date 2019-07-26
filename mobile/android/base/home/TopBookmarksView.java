




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ThumbnailHelper;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Context;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.GridView;

import java.util.Map;





public class TopBookmarksView extends GridView {
    private static final String LOGTAG = "GeckoTopBookmarksView";

    
    private final int mMaxBookmarks;

    
    private final int mNumColumns;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private Map<String, Thumbnail> mThumbnailsCache;

    


    public static class Thumbnail {
        
        private final boolean isThumbnail;

        
        private final Bitmap bitmap;

        public Thumbnail(Bitmap bitmap, boolean isThumbnail) {
            this.bitmap = bitmap;
            this.isThumbnail = isThumbnail;
        }
    }

    public TopBookmarksView(Context context) {
        this(context, null);
    }

    public TopBookmarksView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.topBookmarksViewStyle);
    }

    public TopBookmarksView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mMaxBookmarks = getResources().getInteger(R.integer.number_of_top_sites);
        mNumColumns = getResources().getInteger(R.integer.number_of_top_sites_cols);
        setNumColumns(mNumColumns);
    }

    


    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TopBookmarkItemView row = (TopBookmarkItemView) view;
                String url = row.getUrl();

                if (mUrlOpenListener != null && !TextUtils.isEmpty(url)) {
                    mUrlOpenListener.onUrlOpen(url);
                }
            }
        });
    }

    


    @Override
    public int getColumnWidth() {
        
        
        return (getMeasuredWidth() - getPaddingLeft() - getPaddingRight()) / mNumColumns;
    }

    


    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        final int measuredWidth = getMeasuredWidth();
        final int childWidth = getColumnWidth();
        int childHeight = 0;

        
        ThumbnailHelper.getInstance().setThumbnailWidth(childWidth);

        
        final TopBookmarksAdapter adapter = (TopBookmarksAdapter) getAdapter();
        final int count = (adapter == null ? 0 : adapter.getCount());

        if (adapter != null && count > 0) {
            
            final View child = adapter.getView(0, null, this);
            if (child != null) {
                
                AbsListView.LayoutParams params = (AbsListView.LayoutParams) child.getLayoutParams();
                if (params == null) {
                    params = new AbsListView.LayoutParams(AbsListView.LayoutParams.WRAP_CONTENT,
                                                          AbsListView.LayoutParams.WRAP_CONTENT);
                    child.setLayoutParams(params);
                }

                
                
                int childWidthSpec = MeasureSpec.makeMeasureSpec(childWidth, MeasureSpec.EXACTLY);
                int childHeightSpec = MeasureSpec.makeMeasureSpec(0,  MeasureSpec.UNSPECIFIED);
                child.measure(childWidthSpec, childHeightSpec);
                childHeight = child.getMeasuredHeight();
            }
        }

        
        final int total = Math.min(count > 0 ? count : Integer.MAX_VALUE, mMaxBookmarks);

        
        final int rows = (int) Math.ceil((double) total / mNumColumns);
        final int childrenHeight = childHeight * rows;

        
        final int measuredHeight = childrenHeight + getPaddingTop() + getPaddingBottom();
        setMeasuredDimension(measuredWidth, measuredHeight);
    }

    


    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        
        if (mThumbnailsCache != null) {
            updateThumbnails(mThumbnailsCache);
            mThumbnailsCache = null;
        }
    }

    




    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    




    public void updateThumbnails(Map<String, Thumbnail> thumbnails) {
        if (thumbnails == null) {
            return;
        }

        
        
        if (isLayoutRequested()) {
            mThumbnailsCache = thumbnails;
            return;
        }

        final int count = getAdapter().getCount();
        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);

            
            
            if (child == null) {
                continue;
            }

            TopBookmarkItemView view = (TopBookmarkItemView) child;
            final String url = view.getUrl();

            
            if (TextUtils.isEmpty(url)) {
                view.displayThumbnail(R.drawable.abouthome_thumbnail_add);
            } else {
                
                Thumbnail thumbnail = (thumbnails != null ? thumbnails.get(url) : null);
                if (thumbnail == null) {
                    view.displayThumbnail(null);
                } else if (thumbnail.isThumbnail) {
                    view.displayThumbnail(thumbnail.bitmap);
                } else {
                    view.displayFavicon(thumbnail.bitmap);
                }
            }
        }
    }
}
