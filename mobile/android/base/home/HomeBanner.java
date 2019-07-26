




package org.mozilla.gecko.home;

import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.Property;
import org.mozilla.gecko.animation.PropertyAnimator.PropertyAnimationListener;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.Html;
import android.text.Spanned;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class HomeBanner extends LinearLayout
                        implements GeckoEventListener {
    private static final String LOGTAG = "GeckoHomeBanner";

    final TextView mTextView;
    final ImageView mIconView;
    final ImageButton mCloseButton;

    
    private float mTouchY = -1;

    
    private boolean mSnapBannerToTop;

    
    private boolean mScrollingPages = false;

    
    private boolean mDismissed = false;

    public HomeBanner(Context context) {
        this(context, null);
    }

    public HomeBanner(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater.from(context).inflate(R.layout.home_banner, this);
        mTextView = (TextView) findViewById(R.id.text);
        mIconView = (ImageView) findViewById(R.id.icon);
        mCloseButton = (ImageButton) findViewById(R.id.close);

        mCloseButton.getDrawable().setAlpha(127);
        
        
        mCloseButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                animateDown();
                mDismissed = true;
            }
        });

        setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                
                GeckoAppShell.sendEventToGecko(
                    GeckoEvent.createBroadcastEvent("HomeBanner:Click",(String) getTag()));
            }
        });
    }

    @Override
    public void onAttachedToWindow() {
        GeckoAppShell.getEventDispatcher().registerEventListener("HomeBanner:Data", this);
    }

    @Override
    public void onDetachedFromWindow() {
        GeckoAppShell.getEventDispatcher().unregisterEventListener("HomeBanner:Data", this);
    }

    public void showBanner() {
        if (!mDismissed) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Get", null));
        }
    }

    public void hideBanner() {
        animateDown();
    }

    public void setScrollingPages(boolean scrollingPages) {
        mScrollingPages = scrollingPages;
    }

    @Override
    public void handleMessage(final String event, final JSONObject message) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    
                    setTag(message.getString("id"));
                    setText(message.getString("text"));
                    setIcon(message.optString("iconURI"));
                    animateUp();
                } catch (JSONException e) {
                    Log.e(LOGTAG, "Exception handling " + event + " message", e);
                }
            }
        });
    }

    private void setText(String text) {
        
        final Spanned html = Html.fromHtml(text);

        
        mTextView.setText(html);
    }

    private void setIcon(String iconURI) {
        if (TextUtils.isEmpty(iconURI)) {
            
            mIconView.setVisibility(View.GONE);
            return;
        }

        BitmapUtils.getDrawable(getContext(), iconURI, new BitmapUtils.BitmapLoader() {
            @Override
            public void onBitmapFound(final Drawable d) {
                
                if (d == null) {
                    mIconView.setVisibility(View.GONE);
                    return;
                }

                
                mIconView.setImageDrawable(d);
            }
        });
    }

    private void animateDown() {
        
        if (getVisibility() == GONE && ViewHelper.getTranslationY(this) == getHeight()) {
            return;
        }

        final PropertyAnimator animator = new PropertyAnimator(100);
        animator.attach(this, Property.TRANSLATION_Y, getHeight());
        animator.start();
        animator.addPropertyAnimationListener(new PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {}
            public void onPropertyAnimationEnd() {
                HomeBanner.this.setVisibility(GONE);
            }
        });
    }

    private void animateUp() {
        
        if (getVisibility() == VISIBLE && ViewHelper.getTranslationY(this) == 0) {
            return;
        }

        setVisibility(View.VISIBLE);
        final PropertyAnimator animator = new PropertyAnimator(100);
        animator.attach(this, Property.TRANSLATION_Y, 0);
        animator.start();
    }

    



    public void handleHomeTouch(MotionEvent event) {
        if (mDismissed || mScrollingPages) {
            return;
        }

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                mTouchY = event.getRawY();
                break;
            }

            case MotionEvent.ACTION_MOVE: {
                
                
                if (mTouchY == -1) {
                    mTouchY = event.getRawY();
                    return;
                }

                final float curY = event.getRawY();
                final float delta = mTouchY - curY;
                mSnapBannerToTop = delta <= 0.0f;

                final float height = getHeight();
                float newTranslationY = ViewHelper.getTranslationY(this) + delta;

                
                if (newTranslationY < 0.0f) {
                    newTranslationY = 0.0f;
                } else if (newTranslationY > height) {
                    newTranslationY = height;
                }

                ViewHelper.setTranslationY(this, newTranslationY);
                mTouchY = curY;
                break;
            }

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL: {
                mTouchY = -1;
                final float y = ViewHelper.getTranslationY(this);
                final float height = getHeight();
                if (y > 0.0f && y < height) {
                    if (mSnapBannerToTop) {
                        animateUp();
                    } else {
                        animateDown();
                    }
                }
                break;
            }
        }
    }
}
