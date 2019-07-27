




package org.mozilla.gecko.home;

import org.json.JSONObject;
import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.Property;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.widget.EllipsisTextView;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.Html;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;

public class HomeBanner extends LinearLayout
                        implements GeckoEventListener {
    private static final String LOGTAG = "GeckoHomeBanner";

    
    private float mTouchY = -1;

    
    private boolean mSnapBannerToTop;

    
    private boolean mActive;

    
    private boolean mScrollingPages;

    
    
    private boolean mUserSwipedDown;

    
    
    private final EllipsisTextView mTextView;
    private final ImageView mIconView;

    
    private final float mHeight;

    
    private OnDismissListener mOnDismissListener;

    public interface OnDismissListener {
        public void onDismiss();
    }

    public HomeBanner(Context context) {
        this(context, null);
    }

    public HomeBanner(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.home_banner_content, this);

        mTextView = (EllipsisTextView) findViewById(R.id.text);
        mIconView = (ImageView) findViewById(R.id.icon);

        mHeight = getResources().getDimensionPixelSize(R.dimen.home_banner_height);

        
        setEnabled(false);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        
        
        final ImageButton closeButton = (ImageButton) findViewById(R.id.close);

        
        closeButton.getDrawable().setAlpha(127);

        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                HomeBanner.this.dismiss();

                
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Dismiss", (String) getTag()));
            }
        });

        setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                HomeBanner.this.dismiss();

                
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Click", (String) getTag()));
            }
        });

        EventDispatcher.getInstance().registerGeckoThreadListener(this, "HomeBanner:Data");
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        EventDispatcher.getInstance().unregisterGeckoThreadListener(this, "HomeBanner:Data");
    }

    @Override
    public void setVisibility(int visibility) {
        
        
        if (Versions.preHC && visibility == View.GONE) {
            clearAnimation();
        }

        super.setVisibility(visibility);
    }

    public void setScrollingPages(boolean scrollingPages) {
        mScrollingPages = scrollingPages;
    }

    public void setOnDismissListener(OnDismissListener listener) {
        mOnDismissListener = listener;
    }

    


    private void dismiss() {
        setVisibility(View.GONE);
        setEnabled(false);

        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss();
        }
    }

    


    public void update() {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Get", null));
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        final String id = message.optString("id");
        final String text = message.optString("text");
        final String iconURI = message.optString("iconURI");

        
        if (TextUtils.isEmpty(id) || TextUtils.isEmpty(text)) {
            return;
        }

        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                
                setTag(id);
                mTextView.setOriginalText(Html.fromHtml(text));

                BitmapUtils.getDrawable(getContext(), iconURI, new BitmapUtils.BitmapLoader() {
                    @Override
                    public void onBitmapFound(final Drawable d) {
                        
                        if (d == null) {
                            mIconView.setVisibility(View.GONE);
                        } else {
                            mIconView.setImageDrawable(d);
                            mIconView.setVisibility(View.VISIBLE);
                        }
                    }
                });

                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Shown", id));

                
                setEnabled(true);

                
                if (mActive) {
                    animateUp();
                }
            }
        });
    }

    public void setActive(boolean active) {
        
        if (mActive == active) {
            return;
        }

        mActive = active;

        
        if (!isEnabled()) {
            return;
        }

        if (active) {
            animateUp();
        } else {
            animateDown();
        }
    }

    private void ensureVisible() {
        
        
        if (getVisibility() == View.GONE) {
            
            ViewHelper.setTranslationY(this, mHeight);
            setVisibility(View.VISIBLE);
        }
    }

    private void animateUp() {
        
        if (mUserSwipedDown) {
            return;
        }

        ensureVisible();

        final PropertyAnimator animator = new PropertyAnimator(100);
        animator.attach(this, Property.TRANSLATION_Y, 0);
        animator.start();
    }

    private void animateDown() {
        if (ViewHelper.getTranslationY(this) == mHeight) {
            
            setVisibility(View.GONE);
            return;
        }

        final PropertyAnimator animator = new PropertyAnimator(100);
        animator.attach(this, Property.TRANSLATION_Y, mHeight);
        animator.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
            }

            @Override
            public void onPropertyAnimationEnd() {
                
                setVisibility(View.GONE);
            }
        });
        animator.start();
    }

    public void handleHomeTouch(MotionEvent event) {
        if (!mActive || !isEnabled() || mScrollingPages) {
            return;
        }

        ensureVisible();

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                
                mTouchY = event.getRawY();
                break;
            }

            case MotionEvent.ACTION_MOVE: {
                final float curY = event.getRawY();
                final float delta = mTouchY - curY;
                mSnapBannerToTop = delta <= 0.0f;

                float newTranslationY = ViewHelper.getTranslationY(this) + delta;

                
                if (newTranslationY < 0.0f) {
                    newTranslationY = 0.0f;
                } else if (newTranslationY > mHeight) {
                    newTranslationY = mHeight;
                }

                
                if (delta >= 10 || delta <= -10) {
                    mUserSwipedDown = (newTranslationY == mHeight);
                }

                ViewHelper.setTranslationY(this, newTranslationY);
                mTouchY = curY;
                break;
            }

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL: {
                mTouchY = -1;
                if (mSnapBannerToTop) {
                    animateUp();
                } else {
                    animateDown();
                    mUserSwipedDown = true;
                }
                break;
            }
        }
    }
}
