




package org.mozilla.gecko.home;

import java.lang.ref.WeakReference;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserContract.URLColumns;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.favicons.OnFaviconLoadedListener;
import org.mozilla.gecko.widget.FaviconView;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

public class TwoLinePageRow extends LinearLayout
                            implements Tabs.OnTabsChangedListener {

    protected static final int NO_ICON = 0;

    private final TextView mTitle;
    private final TextView mUrl;

    private int mSwitchToTabIconId;
    private int mPageTypeIconId;

    private final FaviconView mFavicon;

    private boolean mShowIcons;
    private int mLoadFaviconJobId = Favicons.NOT_LOADING;

    
    
    private static class UpdateViewFaviconLoadedListener implements OnFaviconLoadedListener {
        private final WeakReference<FaviconView> view;
        public UpdateViewFaviconLoadedListener(FaviconView view) {
            this.view = new WeakReference<FaviconView>(view);
        }

        @Override
        public void onFaviconLoaded(String url, String faviconURL, Bitmap favicon) {
            FaviconView v = view.get();
            if (v == null) {
                
                return;
            }

            if (favicon == null) {
                v.showDefaultFavicon();
                return;
            }

            v.updateImage(favicon, faviconURL);
        }
    }

    
    private final OnFaviconLoadedListener mFaviconListener;

    
    private String mPageUrl;

    public TwoLinePageRow(Context context) {
        this(context, null);
    }

    public TwoLinePageRow(Context context, AttributeSet attrs) {
        super(context, attrs);

        setGravity(Gravity.CENTER_VERTICAL);

        LayoutInflater.from(context).inflate(R.layout.two_line_page_row, this);
        mTitle = (TextView) findViewById(R.id.title);
        mUrl = (TextView) findViewById(R.id.url);

        mSwitchToTabIconId = NO_ICON;
        mPageTypeIconId = NO_ICON;
        mShowIcons = true;

        mFavicon = (FaviconView) findViewById(R.id.icon);
        mFaviconListener = new UpdateViewFaviconLoadedListener(mFavicon);
    }

    @Override
    protected void onAttachedToWindow() {
        Tabs.registerOnTabsChangedListener(this);
    }

    @Override
    protected void onDetachedFromWindow() {
        
        
        Tabs.unregisterOnTabsChangedListener(this);
    }

    @Override
    public void onTabChanged(final Tab tab, final Tabs.TabEvents msg, final Object data) {
        switch(msg) {
            case ADDED:
            case CLOSED:
            case LOCATION_CHANGE:
                updateDisplayedUrl();
                break;
        }
    }

    private void setTitle(String text) {
        mTitle.setText(text);
    }

    protected void setUrl(String text) {
        mUrl.setText(text);
    }

    protected void setUrl(int stringId) {
        mUrl.setText(stringId);
    }

    protected String getUrl() {
        return mPageUrl;
    }

    protected void setSwitchToTabIcon(int iconId) {
        if (mSwitchToTabIconId == iconId) {
            return;
        }

        mSwitchToTabIconId = iconId;
        mUrl.setCompoundDrawablesWithIntrinsicBounds(mSwitchToTabIconId, 0, mPageTypeIconId, 0);
    }

    private void setPageTypeIcon(int iconId) {
        if (mPageTypeIconId == iconId) {
            return;
        }

        mPageTypeIconId = iconId;
        mUrl.setCompoundDrawablesWithIntrinsicBounds(mSwitchToTabIconId, 0, mPageTypeIconId, 0);
    }

    



    private void updateDisplayedUrl(String url) {
        mPageUrl = url;
        updateDisplayedUrl();
    }

    




    protected void updateDisplayedUrl() {
        boolean isPrivate = Tabs.getInstance().getSelectedTab().isPrivate();
        Tab tab = Tabs.getInstance().getFirstTabForUrl(mPageUrl, isPrivate);
        if (!mShowIcons || tab == null) {
            setUrl(mPageUrl);
            setSwitchToTabIcon(NO_ICON);
        } else {
            setUrl(R.string.switch_to_tab);
            setSwitchToTabIcon(R.drawable.ic_url_bar_tab);
        }
    }

    public void setShowIcons(boolean showIcons) {
        mShowIcons = showIcons;
    }

    public void updateFromCursor(Cursor cursor) {
        if (cursor == null) {
            return;
        }

        int titleIndex = cursor.getColumnIndexOrThrow(URLColumns.TITLE);
        final String title = cursor.getString(titleIndex);

        int urlIndex = cursor.getColumnIndexOrThrow(URLColumns.URL);
        final String url = cursor.getString(urlIndex);

        if (mShowIcons) {
            final int bookmarkIdIndex = cursor.getColumnIndex(Combined.BOOKMARK_ID);
            if (bookmarkIdIndex != -1) {
                final long bookmarkId = cursor.getLong(bookmarkIdIndex);

                
                
                if (bookmarkId == 0) {
                    setPageTypeIcon(NO_ICON);
                } else {
                    setPageTypeIcon(R.drawable.ic_url_bar_star);
                }
            } else {
                setPageTypeIcon(NO_ICON);
            }
        }

        
        
        setTitle(TextUtils.isEmpty(title) ? url : title);

        
        if (url.equals(mPageUrl)) {
            return;
        }

        
        mFavicon.clearImage();
        mLoadFaviconJobId = Favicons.getSizedFaviconForPageFromLocal(getContext(), url, mFaviconListener);

        updateDisplayedUrl(url);
    }
}
