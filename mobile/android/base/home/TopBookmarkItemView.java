




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
import android.widget.LinearLayout;
import android.widget.TextView;







public class TopBookmarkItemView extends RelativeLayout {
    private static final String LOGTAG = "GeckoTopBookmarkItemView";

    
    private static final int[] STATE_EMPTY = { android.R.attr.state_empty };

    
    private final TextView mTitleView;
    private final ImageView mThumbnailView;

    
    private String mTitle;
    private String mUrl;

    
    private boolean mIsPinned = false;

    
    private boolean mIsEmpty = true;

    public TopBookmarkItemView(Context context) {
        this(context, null);
    }

    public TopBookmarkItemView(Context context, AttributeSet attrs) {
        this(context, attrs, R.attr.topBookmarkItemViewStyle);
    }

    public TopBookmarkItemView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        LayoutInflater.from(context).inflate(R.layout.top_bookmark_item_view, this);

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

    


    public boolean isPinned() {
        return mIsPinned;
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

    


    public void setPinned(boolean pinned) {
        mIsPinned = pinned;
        mTitleView.setCompoundDrawablesWithIntrinsicBounds(pinned ? R.drawable.pin : 0, 0, 0, 0);
    }

    




    public void displayThumbnail(int resId) {
        mThumbnailView.setScaleType(ScaleType.CENTER);
        mThumbnailView.setImageResource(resId);
        mThumbnailView.setBackgroundColor(0x0);
    }

    




    public void displayThumbnail(Bitmap thumbnail) {
        if (thumbnail == null) {
            
            displayThumbnail(R.drawable.favicon);
            return;
        }

        mThumbnailView.setScaleType(ScaleType.CENTER_CROP);
        mThumbnailView.setImageBitmap(thumbnail);
        mThumbnailView.setBackgroundDrawable(null);
    }

    




    public void displayFavicon(Bitmap favicon) {
        if (favicon == null) {
            
            displayThumbnail(R.drawable.favicon);
            return;
        }

        mThumbnailView.setScaleType(ScaleType.CENTER);
        mThumbnailView.setImageBitmap(favicon);
        mThumbnailView.setBackgroundColor(Favicons.getFaviconColor(favicon, mUrl));
    }

    



    private void updateTitleView() {
        String title = getTitle();
        if (!TextUtils.isEmpty(title)) {
            mTitleView.setText(title);
            mIsEmpty = false;
        } else {
            mTitleView.setText(R.string.bookmark_add);
            mIsEmpty = true;
        }

        
        refreshDrawableState();
    }
}
