




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ThumbnailHelper;
import org.mozilla.gecko.db.BrowserDB.TopSitesCursorWrapper;
import org.mozilla.gecko.db.BrowserDB.URLColumns;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Context;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.view.animation.GridLayoutAnimationController;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.GridView;

import java.util.EnumSet;





public class TopBookmarksView extends GridView {
    private static final String LOGTAG = "GeckoTopBookmarksView";

    
    public static interface OnPinBookmarkListener {
        public void onPinBookmark(int position);
    }

    
    private final int mMaxBookmarks;

    
    private final int mNumColumns;

    
    private final int mHorizontalSpacing;

    
    private final int mVerticalSpacing;

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private OnPinBookmarkListener mPinBookmarkListener;

    
    private TopBookmarksContextMenuInfo mContextMenuInfo;

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

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TopBookmarksView, defStyle, 0);
        mHorizontalSpacing = a.getDimensionPixelOffset(R.styleable.TopBookmarksView_android_horizontalSpacing, 0x00);
        mVerticalSpacing = a.getDimensionPixelOffset(R.styleable.TopBookmarksView_android_verticalSpacing, 0x00);
        a.recycle();
    }

    


    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TopBookmarkItemView row = (TopBookmarkItemView) view;
                String url = row.getUrl();

                
                
                if (!TextUtils.isEmpty(url)) {
                    if (mUrlOpenListener != null) {
                        mUrlOpenListener.onUrlOpen(url, EnumSet.noneOf(OnUrlOpenListener.Flags.class));
                    }
                } else {
                    if (mPinBookmarkListener != null) {
                        mPinBookmarkListener.onPinBookmark(position);
                    }
                }
            }
        });

        setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                Cursor cursor = (Cursor) parent.getItemAtPosition(position);
                mContextMenuInfo = new TopBookmarksContextMenuInfo(view, position, id, cursor);
                return showContextMenuForChild(TopBookmarksView.this);
            }
        });

        final GridLayoutAnimationController controller = new GridLayoutAnimationController(AnimationUtils.loadAnimation(getContext(), R.anim.grow_fade_in_center));
        setLayoutAnimation(controller);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        mUrlOpenListener = null;
        mPinBookmarkListener = null;
    }

    


    @Override
    public int getColumnWidth() {
        
        
        final int totalHorizontalSpacing = mNumColumns > 0 ? (mNumColumns - 1) * mHorizontalSpacing : 0;
        return (getMeasuredWidth() - getPaddingLeft() - getPaddingRight() - totalHorizontalSpacing) / mNumColumns;
    }

    


    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        final int measuredWidth = getMeasuredWidth();
        final int childWidth = getColumnWidth();
        int childHeight = 0;

        
        ThumbnailHelper.getInstance().setThumbnailWidth(childWidth);

        
        final TopBookmarksAdapter adapter = (TopBookmarksAdapter) getAdapter();
        final int count;

        
        if (adapter == null || (count = adapter.getCount()) == 0) {
            setMeasuredDimension(0, 0);
            return;
        }

        
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

        
        final int total = Math.min(count > 0 ? count : Integer.MAX_VALUE, mMaxBookmarks);

        
        final int rows = (int) Math.ceil((double) total / mNumColumns);
        final int childrenHeight = childHeight * rows;
        final int totalVerticalSpacing = rows > 0 ? (rows - 1) * mVerticalSpacing : 0;

        
        final int measuredHeight = childrenHeight + getPaddingTop() + getPaddingBottom() + totalVerticalSpacing;
        setMeasuredDimension(measuredWidth, measuredHeight);
    }

    @Override
    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    




    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    




    public void setOnPinBookmarkListener(OnPinBookmarkListener listener) {
        mPinBookmarkListener = listener;
    }

    


    public static class TopBookmarksContextMenuInfo extends AdapterContextMenuInfo {

        
        private static final String REGEX_URL_TO_TITLE = "^([a-z]+://)?(www\\.)?";

        public String url;
        public String title;
        public boolean isPinned;

        public TopBookmarksContextMenuInfo(View targetView, int position, long id, Cursor cursor) {
            super(targetView, position, id);

            if (cursor == null) {
                return;
            }

            url = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.URL));
            title = cursor.getString(cursor.getColumnIndexOrThrow(URLColumns.TITLE));
            isPinned = ((TopSitesCursorWrapper) cursor).isPinned();
        }

        public String getDisplayTitle() {
            return TextUtils.isEmpty(title) ? url.replaceAll(REGEX_URL_TO_TITLE, "") : title;
        }
    }
}
