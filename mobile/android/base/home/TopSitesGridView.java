




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.ThumbnailHelper;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.widget.AbsListView;
import android.widget.GridView;





public class TopSitesGridView extends GridView {
    private static final String LOGTAG = "GeckoTopSitesGridView";

    
    public static interface OnEditPinnedSiteListener {
        public void onEditPinnedSite(int position, String searchTerm);
    }

    
    private final int mMaxSites;

    
    private final int mNumColumns;

    
    private final int mHorizontalSpacing;

    
    private final int mVerticalSpacing;

    
    private int mMeasuredWidth;

    
    private int mMeasuredHeight;

    
    private final TopSitesGridItemView dummyChildView;

    
    private TopSitesGridContextMenuInfo mContextMenuInfo;

    
    
    
    private boolean mIsHandlingFocusChange;

    public TopSitesGridView(Context context) {
        this(context, null);
    }

    public TopSitesGridView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.topSitesGridViewStyle);
    }

    public TopSitesGridView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mMaxSites = getResources().getInteger(R.integer.number_of_top_sites);
        mNumColumns = getResources().getInteger(R.integer.number_of_top_sites_cols);
        setNumColumns(mNumColumns);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TopSitesGridView, defStyle, 0);
        mHorizontalSpacing = a.getDimensionPixelOffset(R.styleable.TopSitesGridView_android_horizontalSpacing, 0x00);
        mVerticalSpacing = a.getDimensionPixelOffset(R.styleable.TopSitesGridView_android_verticalSpacing, 0x00);
        a.recycle();

        dummyChildView = new TopSitesGridItemView(context);
        
        AbsListView.LayoutParams params = (AbsListView.LayoutParams) dummyChildView.getLayoutParams();
        if (params == null) {
            params = new AbsListView.LayoutParams(AbsListView.LayoutParams.WRAP_CONTENT,
                    AbsListView.LayoutParams.WRAP_CONTENT);
            dummyChildView.setLayoutParams(params);
        }
    }

    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        mIsHandlingFocusChange = true;
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        mIsHandlingFocusChange = false;
    }

    @Override
    public void requestLayout() {
        if (!mIsHandlingFocusChange) {
            super.requestLayout();
        }
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
        if (measuredWidth == mMeasuredWidth) {
            
            setMeasuredDimension(mMeasuredWidth, mMeasuredHeight);
            return;
        }

        final int columnWidth = getColumnWidth();

        
        
        int childWidthSpec = MeasureSpec.makeMeasureSpec(columnWidth, MeasureSpec.EXACTLY);
        int childHeightSpec = MeasureSpec.makeMeasureSpec(0,  MeasureSpec.UNSPECIFIED);
        dummyChildView.measure(childWidthSpec, childHeightSpec);
        final int childHeight = dummyChildView.getMeasuredHeight();

        
        
        final int thumbnailWidth = dummyChildView.getMeasuredWidth() - dummyChildView.getPaddingLeft() - dummyChildView.getPaddingRight();
        ThumbnailHelper.getInstance().setThumbnailWidth(thumbnailWidth);

        
        final int rows = (int) Math.ceil((double) mMaxSites / mNumColumns);
        final int childrenHeight = childHeight * rows;
        final int totalVerticalSpacing = rows > 0 ? (rows - 1) * mVerticalSpacing : 0;

        
        final int measuredHeight = childrenHeight + getPaddingTop() + getPaddingBottom() + totalVerticalSpacing;
        setMeasuredDimension(measuredWidth, measuredHeight);
        mMeasuredWidth = measuredWidth;
        mMeasuredHeight = measuredHeight;
    }

    @Override
    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    public void setContextMenuInfo(TopSitesGridContextMenuInfo contextMenuInfo) {
        mContextMenuInfo = contextMenuInfo;
    }

    


    public static class TopSitesGridContextMenuInfo extends HomeContextMenuInfo {
        public int type = -1;

        public TopSitesGridContextMenuInfo(View targetView, int position, long id) {
            super(targetView, position, id);
            this.itemType = RemoveItemType.HISTORY;
        }
    }
}
