




package org.mozilla.gecko.toolbar;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.List;

import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.R;
import org.mozilla.gecko.ReaderModeUtils;
import org.mozilla.gecko.SiteIdentity;
import org.mozilla.gecko.SiteIdentity.SecurityMode;
import org.mozilla.gecko.SiteIdentity.MixedMode;
import org.mozilla.gecko.SiteIdentity.TrackingMode;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.toolbar.BrowserToolbarTabletBase.ForwardButtonAnimation;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.StringUtils;
import org.mozilla.gecko.widget.ThemedLinearLayout;
import org.mozilla.gecko.widget.ThemedTextView;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.SystemClock;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.ImageButton;
















public class ToolbarDisplayLayout extends ThemedLinearLayout
                                  implements Animation.AnimationListener {

    private static final String LOGTAG = "GeckoToolbarDisplayLayout";

    
    
    enum UpdateFlags {
        TITLE,
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

    interface OnTitleChangeListener {
        public void onTitleChange(CharSequence title);
    }

    private final BrowserApp mActivity;

    private UIMode mUiMode;

    private boolean mIsAttached;

    private final ThemedTextView mTitle;
    private final int mTitlePadding;
    private ToolbarPrefs mPrefs;
    private OnTitleChangeListener mTitleChangeListener;

    private final ImageButton mSiteSecurity;
    private boolean mSiteSecurityVisible;

    
    private Bitmap mLastFavicon;
    private final ImageButton mFavicon;
    private int mFaviconSize;

    private final ImageButton mStop;
    private OnStopListener mStopListener;

    private final PageActionLayout mPageActionLayout;

    private AlphaAnimation mLockFadeIn;
    private TranslateAnimation mTitleSlideLeft;
    private TranslateAnimation mTitleSlideRight;

    private final SiteIdentityPopup mSiteIdentityPopup;
    private int mSecurityImageLevel;

    private final int LEVEL_SHIELD_ENABLED = 3;
    private final int LEVEL_SHIELD_DISABLED = 4;

    private PropertyAnimator mForwardAnim;

    private final ForegroundColorSpan mUrlColor;
    private final ForegroundColorSpan mBlockedColor;
    private final ForegroundColorSpan mDomainColor;
    private final ForegroundColorSpan mPrivateDomainColor;

    public ToolbarDisplayLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        setOrientation(HORIZONTAL);

        mActivity = (BrowserApp) context;

        LayoutInflater.from(context).inflate(R.layout.toolbar_display_layout, this);

        mTitle = (ThemedTextView) findViewById(R.id.url_bar_title);
        mTitlePadding = mTitle.getPaddingRight();

        final Resources res = getResources();

        mUrlColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_urltext));
        mBlockedColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_blockedtext));
        mDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext));
        mPrivateDomainColor = new ForegroundColorSpan(res.getColor(R.color.url_bar_domaintext_private));

        mFavicon = (ImageButton) findViewById(R.id.favicon);
        mSiteSecurity = (ImageButton) findViewById(R.id.site_security);

        if (HardwareUtils.isTablet()) {
            mSiteSecurity.setVisibility(View.VISIBLE);

            
            
            removeView(mFavicon);
        } else {
            if (Versions.feature16Plus) {
                mFavicon.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            }
            mFaviconSize = Math.round(Favicons.browserToolbarFaviconSize);
        }

        mSiteSecurityVisible = (mSiteSecurity.getVisibility() == View.VISIBLE);

        mSiteIdentityPopup = new SiteIdentityPopup(mActivity);
        mSiteIdentityPopup.setAnchor(this);

        mStop = (ImageButton) findViewById(R.id.stop);
        mPageActionLayout = (PageActionLayout) findViewById(R.id.page_action_layout);
    }

    @Override
    public void onAttachedToWindow() {
        mIsAttached = true;

        Button.OnClickListener faviconListener = new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
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

        float slideWidth = getResources().getDimension(R.dimen.browser_toolbar_site_security_width);

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
    public void onDetachedFromWindow() {
        mIsAttached = false;
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

    void setToolbarPrefs(final ToolbarPrefs prefs) {
        mPrefs = prefs;
    }

    void updateFromTab(Tab tab, EnumSet<UpdateFlags> flags) {
        
        
        if (!mIsAttached) {
            return;
        }

        if (flags.contains(UpdateFlags.TITLE)) {
            updateTitle(tab);
        }

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

    void setTitle(CharSequence title) {
        mTitle.setText(title);

        if (mTitleChangeListener != null) {
            mTitleChangeListener.onTitleChange(title);
        }
    }

    private void updateTitle(Tab tab) {
        
        
        if (tab == null || tab.isEnteringReaderMode()) {
            return;
        }

        final String url = tab.getURL();

        
        
        if (AboutPages.isTitlelessAboutPage(url)) {
            setTitle(null);
            return;
        }

        
        if (tab.getErrorType() == Tab.ErrorType.BLOCKED) {
            final String title = tab.getDisplayTitle();

            final SpannableStringBuilder builder = new SpannableStringBuilder(title);
            builder.setSpan(mBlockedColor, 0, title.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);

            setTitle(builder);
            return;
        }

        
        if (!mPrefs.shouldShowUrl() || url == null) {
            setTitle(tab.getDisplayTitle());
            return;
        }

        String strippedURL = stripAboutReaderURL(url);

        if (mPrefs.shouldTrimUrls()) {
            strippedURL = StringUtils.stripCommonSubdomains(StringUtils.stripScheme(strippedURL));
        }

        CharSequence title = strippedURL;

        final String baseDomain = tab.getBaseDomain();
        if (!TextUtils.isEmpty(baseDomain)) {
            final SpannableStringBuilder builder = new SpannableStringBuilder(title);

            int index = title.toString().indexOf(baseDomain);
            if (index > -1) {
                builder.setSpan(mUrlColor, 0, title.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);
                builder.setSpan(tab.isPrivate() ? mPrivateDomainColor : mDomainColor,
                                index, index + baseDomain.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);

                title = builder;
            }
        }

        setTitle(title);
    }

    private String stripAboutReaderURL(final String url) {
        if (!AboutPages.isAboutReader(url)) {
            return url;
        }

        return ReaderModeUtils.getUrlFromAboutReader(url);
    }

    private void updateFavicon(Tab tab) {
        if (HardwareUtils.isTablet()) {
            
            return;
        }

        if (tab == null) {
            mFavicon.setImageDrawable(null);
            return;
        }

        Bitmap image = tab.getFavicon();

        if (image != null && image == mLastFavicon) {
            Log.d(LOGTAG, "Ignoring favicon: new image is identical to previous one.");
            return;
        }

        
        mLastFavicon = image;

        Log.d(LOGTAG, "updateFavicon(" + image + ")");

        if (image != null) {
            image = Bitmap.createScaledBitmap(image, mFaviconSize, mFaviconSize, false);
            mFavicon.setImageBitmap(image);
        } else {
            mFavicon.setImageResource(R.drawable.favicon_globe);
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
        final MixedMode mixedMode;
        final TrackingMode trackingMode;
        if (siteIdentity == null) {
            securityMode = SecurityMode.UNKNOWN;
            mixedMode = MixedMode.UNKNOWN;
            trackingMode = TrackingMode.UNKNOWN;
        } else {
            securityMode = siteIdentity.getSecurityMode();
            mixedMode = siteIdentity.getMixedMode();
            trackingMode = siteIdentity.getTrackingMode();
        }

        
        
        int imageLevel = securityMode.ordinal();

        
        if (trackingMode == TrackingMode.TRACKING_CONTENT_LOADED ||
            mixedMode == MixedMode.MIXED_CONTENT_LOADED) {
          imageLevel = LEVEL_SHIELD_DISABLED;
        } else if (trackingMode == TrackingMode.TRACKING_CONTENT_BLOCKED ||
                   mixedMode == MixedMode.MIXED_CONTENT_BLOCKED) {
          imageLevel = LEVEL_SHIELD_ENABLED;
        }

        if (mSecurityImageLevel != imageLevel) {
            mSecurityImageLevel = imageLevel;
            mSiteSecurity.setImageLevel(mSecurityImageLevel);
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
            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber start");
        } else {
            Log.i(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - Throbber stop");
        }

        updatePageActions(flags);
    }

    private void updatePageActions(EnumSet<UpdateFlags> flags) {
        final boolean isShowingProgress = (mUiMode == UIMode.PROGRESS);

        mStop.setVisibility(isShowingProgress ? View.VISIBLE : View.GONE);
        mPageActionLayout.setVisibility(!isShowingProgress ? View.VISIBLE : View.GONE);

        boolean shouldShowSiteSecurity = (!isShowingProgress && mSecurityImageLevel > 0);

        setSiteSecurityVisibility(shouldShowSiteSecurity, flags);

        
        
        
        mTitle.setPadding(0, 0, (!isShowingProgress ? mTitlePadding : 0), 0);
    }

    private void setSiteSecurityVisibility(boolean visible, EnumSet<UpdateFlags> flags) {
        
        if (visible == mSiteSecurityVisible || HardwareUtils.isTablet()) {
            return;
        }

        mSiteSecurityVisible = visible;

        mTitle.clearAnimation();
        mSiteSecurity.clearAnimation();

        if (flags.contains(UpdateFlags.DISABLE_ANIMATIONS)) {
            mSiteSecurity.setVisibility(visible ? View.VISIBLE : View.GONE);
            return;
        }

        
        
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

    void setOnStopListener(OnStopListener listener) {
        mStopListener = listener;
    }

    void setOnTitleChangeListener(OnTitleChangeListener listener) {
        mTitleChangeListener = listener;
    }

    






    void updateSiteIdentityAnchor(View view) {
        mSiteIdentityPopup.setAnchor(view);
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
