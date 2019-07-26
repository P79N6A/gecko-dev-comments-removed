




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.TouchEventInterceptor;
import org.mozilla.gecko.util.FloatUtils;

import android.graphics.PointF;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.Log;
import android.view.animation.DecelerateInterpolator;
import android.view.MotionEvent;
import android.view.View;

import java.util.Timer;
import java.util.TimerTask;

public class LayerMarginsAnimator implements TouchEventInterceptor {
    private static final String LOGTAG = "GeckoLayerMarginsAnimator";
    private static final float MS_PER_FRAME = 1000.0f / 60.0f;
    private static final long MARGIN_ANIMATION_DURATION = 250;
    private static final String PREF_SHOW_MARGINS_THRESHOLD = "browser.ui.show-margins-threshold";

    


    private float SHOW_MARGINS_THRESHOLD = 0.20f;

    
    private final RectF mMaxMargins;
    
    private boolean mMarginsPinned;
    
    private Timer mAnimationTimer;
    
    private final DecelerateInterpolator mInterpolator;
    
    private final GeckoLayerClient mTarget;
    

    private final PointF mTouchTravelDistance;
    
    private Integer mPrefObserverId;

    public LayerMarginsAnimator(GeckoLayerClient aTarget, LayerView aView) {
        
        mTarget = aTarget;

        
        mMaxMargins = new RectF();
        mInterpolator = new DecelerateInterpolator();
        mTouchTravelDistance = new PointF();

        
        mPrefObserverId = PrefsHelper.getPref(PREF_SHOW_MARGINS_THRESHOLD, new PrefsHelper.PrefHandlerBase() {
            @Override
            public void prefValue(String pref, int value) {
                SHOW_MARGINS_THRESHOLD = (float)value / 100.0f;
            }

            @Override
            public boolean isObserver() {
                return true;
            }
        });

        
        aView.addTouchInterceptor(this);
    }

    public void destroy() {
        if (mPrefObserverId != null) {
            PrefsHelper.removeObserver(mPrefObserverId);
            mPrefObserverId = null;
        }
    }

    


    public synchronized void setMaxMargins(float left, float top, float right, float bottom) {
        mMaxMargins.set(left, top, right, bottom);

        
        GeckoAppShell.sendEventToGecko(
            GeckoEvent.createBroadcastEvent("Viewport:FixedMarginsChanged",
                "{ \"top\" : " + top + ", \"right\" : " + right
                + ", \"bottom\" : " + bottom + ", \"left\" : " + left + " }"));
    }

    private void animateMargins(final float left, final float top, final float right, final float bottom, boolean immediately) {
        if (mAnimationTimer != null) {
            mAnimationTimer.cancel();
            mAnimationTimer = null;
        }

        if (immediately) {
            ImmutableViewportMetrics newMetrics = mTarget.getViewportMetrics().setMargins(left, top, right, bottom);
            mTarget.forceViewportMetrics(newMetrics, true, true);
            return;
        }

        ImmutableViewportMetrics metrics = mTarget.getViewportMetrics();

        final long startTime = SystemClock.uptimeMillis();
        final float startLeft = metrics.marginLeft;
        final float startTop = metrics.marginTop;
        final float startRight = metrics.marginRight;
        final float startBottom = metrics.marginBottom;

        mAnimationTimer = new Timer("Margin Animation Timer");
        mAnimationTimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                float progress = mInterpolator.getInterpolation(
                    Math.min(1.0f, (SystemClock.uptimeMillis() - startTime)
                                     / (float)MARGIN_ANIMATION_DURATION));

                synchronized(mTarget.getLock()) {
                    ImmutableViewportMetrics oldMetrics = mTarget.getViewportMetrics();
                    ImmutableViewportMetrics newMetrics = oldMetrics.setMargins(
                        FloatUtils.interpolate(startLeft, left, progress),
                        FloatUtils.interpolate(startTop, top, progress),
                        FloatUtils.interpolate(startRight, right, progress),
                        FloatUtils.interpolate(startBottom, bottom, progress));
                    PointF oldOffset = oldMetrics.getMarginOffset();
                    PointF newOffset = newMetrics.getMarginOffset();
                    newMetrics =
                        newMetrics.offsetViewportByAndClamp(newOffset.x - oldOffset.x,
                                                            newOffset.y - oldOffset.y);

                    if (progress >= 1.0f) {
                        if (mAnimationTimer != null) {
                            mAnimationTimer.cancel();
                            mAnimationTimer = null;
                        }

                        
                        mTarget.forceViewportMetrics(newMetrics, true, true);
                    } else {
                        mTarget.forceViewportMetrics(newMetrics, false, false);
                    }
                }
            }
        }, 0, (int)MS_PER_FRAME);
    }

    



    public synchronized void showMargins(boolean immediately) {
        animateMargins(mMaxMargins.left, mMaxMargins.top, mMaxMargins.right, mMaxMargins.bottom, immediately);
    }

    public synchronized void hideMargins(boolean immediately) {
        animateMargins(0, 0, 0, 0, immediately);
    }

    public void setMarginsPinned(boolean pin) {
        if (pin == mMarginsPinned) {
            return;
        }

        mMarginsPinned = pin;
    }

    












    private float scrollMargin(float[] aMargins, float aDelta,
                               float aOverscrollStart, float aOverscrollEnd,
                               float aTouchTravelDistance,
                               float aViewportStart, float aViewportEnd,
                               float aPageStart, float aPageEnd,
                               float aMaxMarginStart, float aMaxMarginEnd,
                               boolean aNegativeOffset) {
        float marginStart = aMargins[0];
        float marginEnd = aMargins[1];
        float viewportSize = aViewportEnd - aViewportStart;
        float exposeThreshold = viewportSize * SHOW_MARGINS_THRESHOLD;

        if (aDelta >= 0) {
            float marginDelta = Math.max(0, aDelta - aOverscrollStart);
            aMargins[0] = marginStart - Math.min(marginDelta, marginStart);
            if (aTouchTravelDistance < exposeThreshold && marginEnd == 0) {
                
                
                marginDelta = Math.max(0, marginDelta - (aPageEnd - aViewportEnd));
            }
            aMargins[1] = marginEnd + Math.min(marginDelta, aMaxMarginEnd - marginEnd);
        } else {
            float marginDelta = Math.max(0, -aDelta - aOverscrollEnd);
            aMargins[1] = marginEnd - Math.min(marginDelta, marginEnd);
            if (-aTouchTravelDistance < exposeThreshold && marginStart == 0) {
                marginDelta = Math.max(0, marginDelta - (aViewportStart - aPageStart));
            }
            aMargins[0] = marginStart + Math.min(marginDelta, aMaxMarginStart - marginStart);
        }

        if (aNegativeOffset) {
            return aDelta - (marginEnd - aMargins[1]);
        }
        return aDelta - (marginStart - aMargins[0]);
    }

    



    ImmutableViewportMetrics scrollBy(ImmutableViewportMetrics aMetrics, float aDx, float aDy) {
        float[] newMarginsX = { aMetrics.marginLeft, aMetrics.marginRight };
        float[] newMarginsY = { aMetrics.marginTop, aMetrics.marginBottom };

        
        if (!mMarginsPinned) {
            
            if (mAnimationTimer != null) {
                mAnimationTimer.cancel();
                mAnimationTimer = null;
            }

            
            if ((aDx >= 0) != (mTouchTravelDistance.x >= 0)) {
                mTouchTravelDistance.x = 0;
            }
            if ((aDy >= 0) != (mTouchTravelDistance.y >= 0)) {
                mTouchTravelDistance.y = 0;
            }

            mTouchTravelDistance.offset(aDx, aDy);
            RectF overscroll = aMetrics.getOverscroll();

            
            if (aMetrics.getPageWidth() >= aMetrics.getWidth()) {
                aDx = scrollMargin(newMarginsX, aDx,
                                   overscroll.left, overscroll.right,
                                   mTouchTravelDistance.x,
                                   aMetrics.viewportRectLeft, aMetrics.viewportRectRight,
                                   aMetrics.pageRectLeft, aMetrics.pageRectRight,
                                   mMaxMargins.left, mMaxMargins.right,
                                   aMetrics.isRTL);
            }
            if (aMetrics.getPageHeight() >= aMetrics.getHeight()) {
                aDy = scrollMargin(newMarginsY, aDy,
                                   overscroll.top, overscroll.bottom,
                                   mTouchTravelDistance.y,
                                   aMetrics.viewportRectTop, aMetrics.viewportRectBottom,
                                   aMetrics.pageRectTop, aMetrics.pageRectBottom,
                                   mMaxMargins.top, mMaxMargins.bottom,
                                   false);
            }
        }

        return aMetrics.setMargins(newMarginsX[0], newMarginsY[0], newMarginsX[1], newMarginsY[1]).offsetViewportBy(aDx, aDy);
    }

    
    @Override
    public boolean onTouch(View view, MotionEvent event) {
        return false;
    }

    
    @Override
    public boolean onInterceptTouchEvent(View view, MotionEvent event) {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN && event.getPointerCount() == 1) {
            mTouchTravelDistance.set(0.0f, 0.0f);
        }

        return false;
    }
}
