




package org.mozilla.gecko.home;

import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.RelativeLayout;
import android.widget.TextView;







public class TopSitesGridItemView extends RelativeLayout {
    private static final String LOGTAG = "GeckoTopSitesGridItemView";

    
    private static final int[] STATE_EMPTY = { android.R.attr.state_empty };

    private static final ScaleType SCALE_TYPE_FAVICON   = ScaleType.CENTER;
    private static final ScaleType SCALE_TYPE_RESOURCE  = ScaleType.CENTER;
    private static final ScaleType SCALE_TYPE_THUMBNAIL = ScaleType.CENTER_CROP;

    
    private final TextView mTitleView;
    private final ImageView mThumbnailView;

    
    private String mTitle;
    private String mUrl;
    private String mFaviconURL;

    private boolean mThumbnailSet;

    
    private boolean mIsPinned = false;

    
    private boolean mIsDirty = false;

    
    private boolean mIsEmpty = true;
    private int mLoadId = Favicons.NOT_LOADING;

    public TopSitesGridItemView(Context context) {
        this(context, null);
    }

    public TopSitesGridItemView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.topSitesGridItemViewStyle);
    }

    public TopSitesGridItemView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        LayoutInflater.from(context).inflate(R.layout.top_sites_grid_item_view, this);

        mTitleView = (TextView) findViewById(R.id.title);
        mThumbnailView = (ImageView) findViewById(R.id.thumbnail);
    }

    


    @Override
    public int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);

        if (mIsEmpty) {
            mergeDrawableStates(drawableState, STATE_EMPTY);
        }

        return drawableState;
    }

    


    public String getTitle() {
        return (!TextUtils.isEmpty(mTitle) ? mTitle : mUrl);
    }

    


    public String getUrl() {
        return mUrl;
    }

    


    public boolean isEmpty() {
        return mIsEmpty;
    }

    


    public void setTitle(String title) {
        if (mTitle != null && mTitle.equals(title)) {
            return;
        }

        mTitle = title;
        updateTitleView();
    }

    


    public void setUrl(String url) {
        if (mUrl != null && mUrl.equals(url)) {
            return;
        }

        mUrl = url;
        updateTitleView();
    }

    public void blankOut() {
        mUrl = "";
        mTitle = "";
        mIsPinned = false;
        updateTitleView();
        mTitleView.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
        setLoadId(Favicons.NOT_LOADING);
        displayThumbnail(R.drawable.top_site_add);
    }

    public void markAsDirty() {
        mIsDirty = true;
    }

    






    public boolean updateState(final String title, final String url, final boolean pinned, final Bitmap thumbnail) {
        boolean changed = false;
        if (mUrl == null || !mUrl.equals(url)) {
            mUrl = url;
            changed = true;
        }

        if (mTitle == null || !mTitle.equals(title)) {
            mTitle = title;
            changed = true;
        }

        if (thumbnail != null) {
            displayThumbnail(thumbnail);
        } else if (changed) {
            
            
            mThumbnailSet = false;
        }

        if (changed) {
            updateTitleView();
            setLoadId(Favicons.NOT_LOADING);
        }

        if (mIsPinned != pinned) {
            mIsPinned = pinned;
            mTitleView.setCompoundDrawablesWithIntrinsicBounds(pinned ? R.drawable.pin : 0, 0, 0, 0);
            changed = true;
        }

        
        
        
        changed = (changed || mIsDirty);
        mIsDirty = false;

        return changed;
    }

    




    public void displayThumbnail(int resId) {
        mThumbnailView.setScaleType(SCALE_TYPE_RESOURCE);
        mThumbnailView.setImageResource(resId);
        mThumbnailView.setBackgroundColor(0x0);
        mThumbnailSet = false;
    }

    




    public void displayThumbnail(Bitmap thumbnail) {
        if (thumbnail == null) {
            
            displayThumbnail(R.drawable.favicon);
            return;
        }
        mThumbnailSet = true;
        Favicons.cancelFaviconLoad(mLoadId);

        mThumbnailView.setScaleType(SCALE_TYPE_THUMBNAIL);
        mThumbnailView.setImageBitmap(thumbnail);
        mThumbnailView.setBackgroundDrawable(null);
    }

    public void displayFavicon(Bitmap favicon, String faviconURL, int expectedLoadId) {
        if (mLoadId != Favicons.NOT_LOADING &&
            mLoadId != expectedLoadId) {
            
            return;
        }

        
        displayFavicon(favicon, faviconURL);
    }

    




    public void displayFavicon(Bitmap favicon, String faviconURL) {
        if (mThumbnailSet) {
            
            return;
        }

        if (favicon == null) {
            
            displayThumbnail(R.drawable.favicon);
            return;
        }

        if (faviconURL != null) {
            mFaviconURL = faviconURL;
        }

        mThumbnailView.setScaleType(SCALE_TYPE_FAVICON);
        mThumbnailView.setImageBitmap(favicon);

        if (mFaviconURL != null) {
            mThumbnailView.setBackgroundColor(Favicons.getFaviconColor(mFaviconURL));
        }
    }

    



    private void updateTitleView() {
        String title = getTitle();
        if (!TextUtils.isEmpty(title)) {
            mTitleView.setText(title);
            mIsEmpty = false;
        } else {
            mTitleView.setText(R.string.home_top_sites_add);
            mIsEmpty = true;
        }

        
        refreshDrawableState();
    }

    public void setLoadId(int aLoadId) {
        Favicons.cancelFaviconLoad(mLoadId);
        mLoadId = aLoadId;
    }
}
