




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.R;
import org.mozilla.gecko.SiteIdentity;
import org.mozilla.gecko.SiteIdentity.SecurityMode;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.toolbar.BrowserToolbar.ForwardButtonAnimation;
import org.mozilla.gecko.widget.GeckoLinearLayout;
import org.mozilla.gecko.widget.GeckoTextView;

import org.json.JSONObject;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Build;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.AlphaAnimation;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout.LayoutParams;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.List;

public class ToolbarDisplayLayout extends GeckoLinearLayout
                                  implements Animation.AnimationListener {

    private static final String LOGTAG = "GeckoToolbarDisplayLayout";

    enum UpdateFlags {
        FAVICON,
        PROGRESS,
        SITE_IDENTITY,
        PRIVATE_MODE,
        DISABLE_ANIMATIONS
    }

    private enum UIMode {
        PROGRESS,
        DISPLAY
    }

    interface OnStopListener {
        public Tab onStop();
    }

    final private BrowserApp mActivity;

    private UIMode mUiMode;

    private GeckoTextView mTitle;
    private int mTitlePadding;

    private ImageButton mSiteSecurity;
    private boolean mSiteSecurityVisible;

    
    private Bitmap mLastFavicon;
    private ImageButton mFavicon;
    private int mFaviconSize;

    private ImageButton mStop;
    private OnStopListener mStopListener;

    private PageActionLayout mPageActionLayout;

    private Animation mProgressSpinner;

    private AlphaAnimation mLockFadeIn;
    private TranslateAnimation mTitleSlideLeft;
    private TranslateAnimation mTitleSlideRight;

    private SiteIdentityPopup mSiteIdentityPopup;
    private SecurityMode mSecurityMode;

    private PropertyAnimator mForwardAnim;

    public ToolbarDisplayLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        setOrientation(HORIZONTAL);

        mActivity = (BrowserApp) context;

        LayoutInflater.from(context).inflate(R.layout.toolbar_display_layout, this);

        mTitle = (GeckoTextView) findViewById(R.id.url_bar_title);
        mTitlePadding = mTitle.getPaddingRight();

        Resources res = getResources();

        mFavicon = (ImageButton) findViewById(R.id.favicon);
        if (Build.VERSION.SDK_INT >= 11) {
            if (Build.VERSION.SDK_INT >= 16) {
                mFavicon.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            }
            mFavicon.setLayerType(View.LAYER_TYPE_HARDWARE, null);
        }
        mFaviconSize = Math.round(res.getDimension(R.dimen.browser_toolbar_favicon_size));

        mSiteSecurity = (ImageButton) findViewById(R.id.site_security);
        mSiteSecurityVisible = (mSiteSecurity.getVisibility() == View.VISIBLE);

        mSiteIdentityPopup = new SiteIdentityPopup(mActivity);
        mSiteIdentityPopup.setAnchor(mSiteSecurity);

        mProgressSpinner = AnimationUtils.loadAnimation(mActivity, R.anim.progress_spinner);

        mStop = (ImageButton) findViewById(R.id.stop);
        mPageActionLayout = (PageActionLayout) findViewById(R.id.page_action_layout);
    }

    @Override
    public void onAttachedToWindow() {
        Button.OnClickListener faviconListener = new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mSiteSecurity.getVisibility() != View.VISIBLE) {
                    return;
                }

                mSiteIdentityPopup.show();
            }
        };

        mFavicon.setOnClickListener(faviconListener);
        mSiteSecurity.setOnClickListener(faviconListener);

        mStop.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mStopListener != null) {
                    
                    
                    final Tab tab = mStopListener.onStop();
                    if (tab != null) {
                        updateUiMode(tab, UIMode.DISPLAY, EnumSet.noneOf(UpdateFlags.class));
                    }
                }
            }
        });

        float slideWidth = getResources().getDimension(R.dimen.browser_toolbar_lock_width);

        LayoutParams siteSecParams = (LayoutParams) mSiteSecurity.getLayoutParams();
        final float scale = getResources().getDisplayMetrics().density;
        slideWidth += (siteSecParams.leftMargin + siteSecParams.rightMargin) * scale + 0.5f;

        mLockFadeIn = new AlphaAnimation(0.0f, 1.0f);
        mLockFadeIn.setAnimationListener(this);

        mTitleSlideLeft = new TranslateAnimation(slideWidth, 0, 0, 0);
        mTitleSlideLeft.setAnimationListener(this);

        mTitleSlideRight = new TranslateAnimation(-slideWidth, 0, 0, 0);
        mTitleSlideRight.setAnimationListener(this);

        final int lockAnimDuration = 300;
        mLockFadeIn.setDuration(lockAnimDuration);
        mTitleSlideLeft.setDuration(lockAnimDuration);
        mTitleSlideRight.setDuration(lockAnimDuration);
    }

    @Override
    public void onAnimationStart(Animation animation) {
        if (animation.equals(mLockFadeIn)) {
            if (mSiteSecurityVisible)
                mSiteSecurity.setVisibility(View.VISIBLE);
        } else if (animation.equals(mTitleSlideLeft)) {
            
            
            
            mSiteSecurity.setVisibility(View.GONE);
        } else if (animation.equals(mTitleSlideRight)) {
            
            
            mSiteSecurity.setVisibility(View.INVISIBLE);
        }
    }

    @Override
    public void onAnimationRepeat(Animation animation) {
    }

    @Override
    public void onAnimationEnd(Animation animation) {
        if (animation.equals(mTitleSlideRight)) {
            mSiteSecurity.startAnimation(mLockFadeIn);
        }
    }

    @Override
    public void setNextFocusDownId(int nextId) {
        mFavicon.setNextFocusDownId(nextId);
        mStop.setNextFocusDownId(nextId);
        mSiteSecurity.setNextFocusDownId(nextId);
        mPageActionLayout.setNextFocusDownId(nextId);
    }

    void updateFromTab(Tab tab, EnumSet<UpdateFlags> flags) {
        if (flags.contains(UpdateFlags.FAVICON)) {
            updateFavicon(tab);
        }

        if (flags.contains(UpdateFlags.SITE_IDENTITY)) {
            updateSiteIdentity(tab, flags);
        }

        if (flags.contains(UpdateFlags.PROGRESS)) {
            updateProgress(tab, flags);
        }

        if (flags.contains(UpdateFlags.PRIVATE_MODE)) {
            mTitle.setPrivateMode(tab != null && tab.isPrivate());
        }
    }

    private void updateFavicon(Tab tab) {
        if (tab == null) {
            mFavicon.setImageDrawable(null);
            return;
        }

        if (tab.getState() == Tab.STATE_LOADING) {
            return;
        }

        Bitmap image = tab.getFavicon();
        if (image == mLastFavicon) {
            Log.d(LOGTAG, "Ignoring favicon: new image is identical to previous one.");
            return;
        }

        
        mLastFavicon = image;

        Log.d(LOGTAG, "updateFavicon(" + image + ")");

        if (image != null) {
            image = Bitmap.createScaledBitmap(image, mFaviconSize, mFaviconSize, false);
            mFavicon.setImageBitmap(image);
        } else {
            mFavicon.setImageDrawable(null);            
        }
    }

    private void updateSiteIdentity(Tab tab, EnumSet<UpdateFlags> flags) {
        final SiteIdentity siteIdentity;
        if (tab == null) {
            siteIdentity = null;
        } else {
            siteIdentity = tab.getSiteIdentity();
        }

        mSiteIdentityPopup.setSiteIdentity(siteIdentity);

        final SecurityMode securityMode;
        if (siteIdentity == null) {
            securityMode = SecurityMode.UNKNOWN;
        } else {
            securityMode = siteIdentity.getSecurityMode();
        }

        if (mSecurityMode != securityMode) {
            mSecurityMode = securityMode;
            mSiteSecurity.setImageLevel(mSecurityMode.ordinal());
            updatePageActions(flags);
        }
    }

    private void updateProgress(Tab tab, EnumSet<UpdateFlags> flags) {
        final boolean shouldShowThrobber = (tab != null &&
                                            tab.getState() == Tab.STATE_LOADING);

        updateUiMode(tab, shouldShowThrobber ? UIMode.PROGRESS : UIMode.DISPLAY, flags);
    }

    private void updateUiMode(Tab tab, UIMode uiMode, EnumSet<UpdateFlags> flags) {
        if (mUiMode == uiMode) {
            return;
        }

        mUiMode = uiMode;

        
        
        
        if (mUiMode == UIMode.PROGRESS) {
            mLastFavicon = null;
            mFavicon.setImageResource(R.drawable.progress_spinner);
            mFavicon.setAnimation(mProgressSpinner);
            mProgressSpinner.start();

            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber start");
        } else {
            updateFavicon(tab);
            mFavicon.setAnimation(null);
            mProgressSpinner.cancel();

            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber stop");
        }

        updatePageActions(flags);
    }

    private void updatePageActions(EnumSet<UpdateFlags> flags) {
        final boolean isShowingProgress = (mUiMode == UIMode.PROGRESS);

        mStop.setVisibility(isShowingProgress ? View.VISIBLE : View.GONE);
        mPageActionLayout.setVisibility(!isShowingProgress ? View.VISIBLE : View.GONE);

        boolean shouldShowSiteSecurity = (!isShowingProgress &&
                                          mSecurityMode != SecurityMode.UNKNOWN);

        setSiteSecurityVisibility(shouldShowSiteSecurity, flags);

        
        
        
        mTitle.setPadding(0, 0, (!isShowingProgress ? mTitlePadding : 0), 0);
    }

    private void setSiteSecurityVisibility(boolean visible, EnumSet<UpdateFlags> flags) {
        if (visible == mSiteSecurityVisible) {
            return;
        }

        mSiteSecurityVisible = visible;

        if (flags.contains(UpdateFlags.DISABLE_ANIMATIONS)) {
            mSiteSecurity.setVisibility(visible ? View.VISIBLE : View.GONE);
            return;
        }

        mTitle.clearAnimation();
        mSiteSecurity.clearAnimation();

        
        
        mLockFadeIn.reset();
        mTitleSlideLeft.reset();
        mTitleSlideRight.reset();

        if (mForwardAnim != null) {
            long delay = mForwardAnim.getRemainingTime();
            mTitleSlideRight.setStartOffset(delay);
            mTitleSlideLeft.setStartOffset(delay);
        } else {
            mTitleSlideRight.setStartOffset(0);
            mTitleSlideLeft.setStartOffset(0);
        }

        mTitle.startAnimation(visible ? mTitleSlideRight : mTitleSlideLeft);
    }

    List<View> getFocusOrder() {
        return Arrays.asList(mSiteSecurity, mPageActionLayout, mStop);
    }

    void setTitle(CharSequence title) {
        mTitle.setText(title);
    }

    void setOnStopListener(OnStopListener listener) {
        mStopListener = listener;
    }

    View getDoorHangerAnchor() {
        return mFavicon;
    }

    void prepareForwardAnimation(PropertyAnimator anim, ForwardButtonAnimation animation, int width) {
        mForwardAnim = anim;

        if (animation == ForwardButtonAnimation.HIDE) {
            anim.attach(mTitle,
                        PropertyAnimator.Property.TRANSLATION_X,
                        0);
            anim.attach(mFavicon,
                        PropertyAnimator.Property.TRANSLATION_X,
                        0);
            anim.attach(mSiteSecurity,
                        PropertyAnimator.Property.TRANSLATION_X,
                        0);

            
            
            
            ViewHelper.setTranslationX(mTitle, width);
            ViewHelper.setTranslationX(mFavicon, width);
            ViewHelper.setTranslationX(mSiteSecurity, width);
        } else {
            anim.attach(mTitle,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
            anim.attach(mFavicon,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
            anim.attach(mSiteSecurity,
                        PropertyAnimator.Property.TRANSLATION_X,
                        width);
        }
    }

    void finishForwardAnimation() {
        ViewHelper.setTranslationX(mTitle, 0);
        ViewHelper.setTranslationX(mFavicon, 0);
        ViewHelper.setTranslationX(mSiteSecurity, 0);

        mForwardAnim = null;
    }

    void prepareStartEditingAnimation() {
        
        ViewHelper.setAlpha(mPageActionLayout, 0);
        ViewHelper.setAlpha(mStop, 0);
    }

    void prepareStopEditingAnimation(PropertyAnimator anim) {
        
        
        anim.attach(mPageActionLayout,
                    PropertyAnimator.Property.ALPHA,
                    1);

        anim.attach(mStop,
                    PropertyAnimator.Property.ALPHA,
                    1);
    }

    boolean dismissSiteIdentityPopup() {
        if (mSiteIdentityPopup != null && mSiteIdentityPopup.isShowing()) {
            mSiteIdentityPopup.dismiss();
            return true;
        }

        return false;
    }
}