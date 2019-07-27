




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserContract.TopSites;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.ImageView.ScaleType;
import android.widget.RelativeLayout;
import android.widget.TextView;







public class TopSitesGridItemView extends RelativeLayout {
    private static final String LOGTAG = "GeckoTopSitesGridItemView";

    
    private static final int[] STATE_EMPTY = { android.R.attr.state_empty };

    private static final ScaleType SCALE_TYPE_FAVICON   = ScaleType.CENTER;
    private static final ScaleType SCALE_TYPE_RESOURCE  = ScaleType.CENTER;
    private static final ScaleType SCALE_TYPE_THUMBNAIL = ScaleType.CENTER_CROP;
    private static final ScaleType SCALE_TYPE_URL       = ScaleType.CENTER_INSIDE;

    
    private final TextView mTitleView;
    private final TopSitesThumbnailView mThumbnailView;

    
    private String mTitle;
    private String mUrl;
    private String mFaviconURL;

    private boolean mThumbnailSet;

    
    private int mType = -1;

    
    private boolean mIsDirty;

    
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
        mThumbnailView = (TopSitesThumbnailView) findViewById(R.id.thumbnail);
    }

    


    @Override
    public int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);

        if (mType == TopSites.TYPE_BLANK) {
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

    


    public int getType() {
        return mType;
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
        updateType(TopSites.TYPE_BLANK);
        updateTitleView();
        setLoadId(Favicons.NOT_LOADING);
        ImageLoader.with(getContext()).cancelRequest(mThumbnailView);
        displayThumbnail(R.drawable.top_site_add);

    }

    public void markAsDirty() {
        mIsDirty = true;
    }

    






    public boolean updateState(final String title, final String url, final int type, final TopSitesPanel.ThumbnailInfo thumbnail) {
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
            if (thumbnail.imageUrl != null) {
                displayThumbnail(thumbnail.imageUrl, thumbnail.bgColor);
            } else if (thumbnail.bitmap != null) {
                displayThumbnail(thumbnail.bitmap);
            }
        } else if (changed) {
            
            
            mThumbnailSet = false;
        }

        if (changed) {
            updateTitleView();
            setLoadId(Favicons.NOT_LOADING);
            ImageLoader.with(getContext()).cancelRequest(mThumbnailView);
        }

        if (updateType(type)) {
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
        ImageLoader.with(getContext()).cancelRequest(mThumbnailView);

        mThumbnailView.setScaleType(SCALE_TYPE_THUMBNAIL);
        mThumbnailView.setImageBitmap(thumbnail);
        mThumbnailView.setBackgroundDrawable(null);
    }

    





    public void displayThumbnail(final String imageUrl, final int bgColor) {
        mThumbnailView.setScaleType(SCALE_TYPE_URL);
        mThumbnailView.setBackgroundColor(bgColor);
        mThumbnailSet = true;

        ImageLoader.with(getContext())
                   .load(imageUrl)
                   .noFade()
                   .error(R.drawable.favicon)
                   .into(mThumbnailView);
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
            final int bgColor = Favicons.getFaviconColor(mFaviconURL);
            mThumbnailView.setBackgroundColorWithOpacityFilter(bgColor);
        }
    }

    



    private boolean updateType(int type) {
        if (mType == type) {
            return false;
        }

        mType = type;
        refreshDrawableState();

        int pinResourceId = (type == TopSites.TYPE_PINNED ? R.drawable.pin : 0);
        mTitleView.setCompoundDrawablesWithIntrinsicBounds(pinResourceId, 0, 0, 0);

        return true;
    }

    



    private void updateTitleView() {
        String title = getTitle();
        if (!TextUtils.isEmpty(title)) {
            mTitleView.setText(title);
        } else {
            mTitleView.setText(R.string.home_top_sites_add);
        }
    }

    public void setLoadId(int aLoadId) {
        Favicons.cancelFaviconLoad(mLoadId);
        mLoadId = aLoadId;
    }
}
