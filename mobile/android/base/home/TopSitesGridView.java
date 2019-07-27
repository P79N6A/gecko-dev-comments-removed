




package org.mozilla.gecko.home;

import java.util.EnumSet;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;
import org.mozilla.gecko.ThumbnailHelper;
import org.mozilla.gecko.db.BrowserContract.TopSites;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.util.StringUtils;

import android.content.Context;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;
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

    
    private OnUrlOpenListener mUrlOpenListener;

    
    private OnEditPinnedSiteListener mEditPinnedSiteListener;

    
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
    }

    


    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TopSitesGridItemView item = (TopSitesGridItemView) view;

                
                String url = StringUtils.decodeUserEnteredUrl(item.getUrl());
                int type = item.getType();

                
                
                if (type != TopSites.TYPE_BLANK) {
                    if (mUrlOpenListener != null) {
                        final TelemetryContract.Method method;
                        if (type == TopSites.TYPE_SUGGESTED) {
                            method = TelemetryContract.Method.SUGGESTION;
                        } else {
                            method = TelemetryContract.Method.GRID_ITEM;
                        }
                        Telemetry.sendUIEvent(TelemetryContract.Event.LOAD_URL, method, Integer.toString(position));

                        mUrlOpenListener.onUrlOpen(url, EnumSet.noneOf(OnUrlOpenListener.Flags.class));
                    }
                } else {
                    if (mEditPinnedSiteListener != null) {
                        mEditPinnedSiteListener.onEditPinnedSite(position, "");
                    }
                }
            }
        });

        setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                Cursor cursor = (Cursor) parent.getItemAtPosition(position);

                TopSitesGridItemView item = (TopSitesGridItemView) view;
                if (cursor == null || item.getType() == TopSites.TYPE_BLANK) {
                    mContextMenuInfo = null;
                    return false;
                }

                mContextMenuInfo = new TopSitesGridContextMenuInfo(view, position, id);
                updateContextMenuFromCursor(mContextMenuInfo, cursor);
                return showContextMenuForChild(TopSitesGridView.this);
            }
        });
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        mUrlOpenListener = null;
        mEditPinnedSiteListener = null;
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

        
        final TopSitesGridItemView child = new TopSitesGridItemView(getContext());

        
        AbsListView.LayoutParams params = (AbsListView.LayoutParams) child.getLayoutParams();
        if (params == null) {
            params = new AbsListView.LayoutParams(AbsListView.LayoutParams.WRAP_CONTENT,
                                                  AbsListView.LayoutParams.WRAP_CONTENT);
            child.setLayoutParams(params);
        }

        
        
        int childWidthSpec = MeasureSpec.makeMeasureSpec(columnWidth, MeasureSpec.EXACTLY);
        int childHeightSpec = MeasureSpec.makeMeasureSpec(0,  MeasureSpec.UNSPECIFIED);
        child.measure(childWidthSpec, childHeightSpec);
        final int childHeight = child.getMeasuredHeight();

        
        
        final int thumbnailWidth = child.getMeasuredWidth() - child.getPaddingLeft() - child.getPaddingRight();
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

    






    private void updateContextMenuFromCursor(TopSitesGridContextMenuInfo info, Cursor cursor) {
        info.url = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.URL));
        info.title = cursor.getString(cursor.getColumnIndexOrThrow(TopSites.TITLE));
        info.type = cursor.getInt(cursor.getColumnIndexOrThrow(TopSites.TYPE));
        info.historyId = cursor.getInt(cursor.getColumnIndexOrThrow(TopSites.HISTORY_ID));
    }
    




    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    




    public void setOnEditPinnedSiteListener(final OnEditPinnedSiteListener listener) {
        mEditPinnedSiteListener = listener;
    }

    


    public static class TopSitesGridContextMenuInfo extends HomeContextMenuInfo {
        public int type = -1;

        public TopSitesGridContextMenuInfo(View targetView, int position, long id) {
            super(targetView, position, id);
            this.itemType = RemoveItemType.HISTORY;
        }
    }
}
