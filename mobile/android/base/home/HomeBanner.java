




package org.mozilla.gecko.home;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.Property;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Build;
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

    
    private float mTouchY = -1;

    
    private boolean mSnapBannerToTop;

    
    private boolean mActive = false;

    
    private boolean mScrollingPages = false;

    
    
    private boolean mUserSwipedDown = false;

    private final TextView mTextView;
    private final ImageView mIconView;

    
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

        mTextView = (TextView) findViewById(R.id.text);
        mIconView = (ImageView) findViewById(R.id.icon);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();

        
        
        final ImageButton closeButton = (ImageButton) findViewById(R.id.close);

        
        closeButton.getDrawable().setAlpha(127);

        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                HomeBanner.this.setVisibility(View.GONE);

                
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Dismiss", (String) getTag()));

                if (mOnDismissListener != null) {
                    mOnDismissListener.onDismiss();
                }
            }
        });

        setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                
                
                HomeBanner.this.setVisibility(View.GONE);

                
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Click", (String) getTag()));
            }
        });

        GeckoAppShell.getEventDispatcher().registerEventListener("HomeBanner:Data", this);
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();

        GeckoAppShell.getEventDispatcher().unregisterEventListener("HomeBanner:Data", this);
    }

    @Override
    public void setVisibility(int visibility) {
        
        
        if (Build.VERSION.SDK_INT < 11 && visibility == View.GONE) {
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

    


    public void update() {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Get", null));
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        final String id = message.optString("id");
        final String text = message.optString("text");
        final String iconURI = message.optString("iconURI");

        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                
                if (TextUtils.isEmpty(id) || TextUtils.isEmpty(text)) {
                    setVisibility(View.GONE);
                    return;
                }

                
                setTag(id);
                mTextView.setText(Html.fromHtml(text));

                BitmapUtils.getDrawable(getContext(), iconURI, new BitmapUtils.BitmapLoader() {
                    @Override
                    public void onBitmapFound(final Drawable d) {
                        
                        if (d == null) {
                            mIconView.setVisibility(View.GONE);
                        } else {
                            mIconView.setImageDrawable(d);
                        }
                    }
                });

                setVisibility(View.VISIBLE);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomeBanner:Shown", id));

                
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

        
        if (getVisibility() != View.VISIBLE) {
            return;
        }

        if (active) {
            animateUp();
        } else {
            animateDown();
        }
    }

    private void animateUp() {
        
        
        if (ViewHelper.getTranslationY(this) == 0 || mUserSwipedDown) {
            return;
        }

        final PropertyAnimator animator = new PropertyAnimator(100);
        animator.attach(this, Property.TRANSLATION_Y, 0);
        animator.start();
    }

    private void animateDown() {
        
        if (ViewHelper.getTranslationY(this) == getHeight()) {
            return;
        }

        final PropertyAnimator animator = new PropertyAnimator(100);
        animator.attach(this, Property.TRANSLATION_Y, getHeight());
        animator.start();
    }

    public void handleHomeTouch(MotionEvent event) {
        if (!mActive || getVisibility() == GONE || mScrollingPages) {
            return;
        }

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                
                mTouchY = event.getRawY();
                break;
            }

            case MotionEvent.ACTION_MOVE: {
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

                
                if (delta >= 10 || delta <= -10) {
                    mUserSwipedDown = newTranslationY == height;
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
                        mUserSwipedDown = true;
                    }
                }
                break;
            }
        }
    }
}
