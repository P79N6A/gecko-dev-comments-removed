




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;

import android.content.Context;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ListView;





public class HomeListView extends ListView
                          implements OnItemLongClickListener {

    
    private HomeContextMenuInfo mContextMenuInfo;

    
    protected OnUrlOpenListener mUrlOpenListener;

    
    private final boolean mShowTopDivider;

    
    private HomeContextMenuInfo.Factory mContextMenuInfoFactory;

    public HomeListView(Context context) {
        this(context, null);
    }

    public HomeListView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.homeListViewStyle);
    }

    public HomeListView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.HomeListView, defStyle, 0);
        mShowTopDivider = a.getBoolean(R.styleable.HomeListView_topDivider, false);
        a.recycle();

        setOnItemLongClickListener(this);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        final Drawable divider = getDivider();
        if (mShowTopDivider && divider != null) {
            final int dividerHeight = getDividerHeight();
            final View view = new View(getContext());
            view.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, dividerHeight));
            addHeaderView(view);
        }
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        mUrlOpenListener = null;
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        Object item = parent.getItemAtPosition(position);

        
        if (item instanceof Cursor) {
            Cursor cursor = (Cursor) item;
            if (cursor == null || mContextMenuInfoFactory == null) {
                mContextMenuInfo = null;
                return false;
            }

            mContextMenuInfo = mContextMenuInfoFactory.makeInfoForCursor(view, position, id, cursor);
            return showContextMenuForChild(HomeListView.this);

        } else if (mContextMenuInfoFactory instanceof HomeContextMenuInfo.ListFactory) {
            mContextMenuInfo = ((HomeContextMenuInfo.ListFactory) mContextMenuInfoFactory).makeInfoForAdapter(view, position, id, getAdapter());
            return showContextMenuForChild(HomeListView.this);
        } else {
            mContextMenuInfo = null;
            return false;
        }
    }

    @Override
    public ContextMenuInfo getContextMenuInfo() {
        return mContextMenuInfo;
    }

    @Override
    public void setOnItemClickListener(final AdapterView.OnItemClickListener listener) {
        if (listener == null) {
            super.setOnItemClickListener(null);
            return;
        }

        super.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (mShowTopDivider) {
                    position--;
                }

                listener.onItemClick(parent, view, position, id);
            }
        });
    }

    public void setContextMenuInfoFactory(final HomeContextMenuInfo.Factory factory) {
        mContextMenuInfoFactory = factory;
    }

    public OnUrlOpenListener getOnUrlOpenListener() {
        return mUrlOpenListener;
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }
}
