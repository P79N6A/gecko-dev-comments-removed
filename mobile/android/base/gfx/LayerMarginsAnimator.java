




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.util.FloatUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.graphics.PointF;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.Log;
import android.view.animation.DecelerateInterpolator;
import android.view.MotionEvent;
import android.view.View;

public class LayerMarginsAnimator {
    private static final String LOGTAG = "GeckoLayerMarginsAnimator";
    
    private static final long MARGIN_ANIMATION_DURATION = 250000000;
    private static final String PREF_SHOW_MARGINS_THRESHOLD = "browser.ui.show-margins-threshold";

    


    private float SHOW_MARGINS_THRESHOLD = 0.20f;

    


    private final RectF mMaxMargins;
    
    private boolean mMarginsPinned;
    
    private LayerMarginsAnimationTask mAnimationTask;
    
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
    }

    public void destroy() {
        if (mPrefObserverId != null) {
            PrefsHelper.removeObserver(mPrefObserverId);
            mPrefObserverId = null;
        }
    }

    


    public synchronized void setMaxMargins(float left, float top, float right, float bottom) {
        ThreadUtils.assertOnUiThread();

        mMaxMargins.set(left, top, right, bottom);

        
        GeckoAppShell.sendEventToGecko(
            GeckoEvent.createBroadcastEvent("Viewport:FixedMarginsChanged",
                "{ \"top\" : " + top + ", \"right\" : " + right
                + ", \"bottom\" : " + bottom + ", \"left\" : " + left + " }"));
    }

    RectF getMaxMargins() {
        return mMaxMargins;
    }

    private void animateMargins(final float left, final float top, final float right, final float bottom, boolean immediately) {
        if (mAnimationTask != null) {
            mTarget.getView().removeRenderTask(mAnimationTask);
            mAnimationTask = null;
        }

        if (immediately) {
            ImmutableViewportMetrics newMetrics = mTarget.getViewportMetrics().setMargins(left, top, right, bottom);
            mTarget.forceViewportMetrics(newMetrics, true, true);
            return;
        }

        ImmutableViewportMetrics metrics = mTarget.getViewportMetrics();

        mAnimationTask = new LayerMarginsAnimationTask(false, metrics, left, top, right, bottom);
        mTarget.getView().postRenderTask(mAnimationTask);
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

    public boolean areMarginsShown() {
        final ImmutableViewportMetrics metrics = mTarget.getViewportMetrics();
        return metrics.marginLeft != 0  ||
               metrics.marginRight != 0 ||
               metrics.marginTop != 0   ||
               metrics.marginBottom != 0;
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
            
            if (mAnimationTask != null) {
                mTarget.getView().removeRenderTask(mAnimationTask);
                mAnimationTask = null;
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

    boolean onInterceptTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN && event.getPointerCount() == 1) {
            mTouchTravelDistance.set(0.0f, 0.0f);
        }

        return false;
    }

    class LayerMarginsAnimationTask extends RenderTask {
        private float mStartLeft, mStartTop, mStartRight, mStartBottom;
        private float mTop, mBottom, mLeft, mRight;
        private boolean mContinueAnimation;

        public LayerMarginsAnimationTask(boolean runAfter, ImmutableViewportMetrics metrics,
                float left, float top, float right, float bottom) {
            super(runAfter);
            mContinueAnimation = true;
            this.mStartLeft = metrics.marginLeft;
            this.mStartTop = metrics.marginTop;
            this.mStartRight = metrics.marginRight;
            this.mStartBottom = metrics.marginBottom;
            this.mLeft = left;
            this.mRight = right;
            this.mTop = top;
            this.mBottom = bottom;
        }

        @Override
        public boolean internalRun(long timeDelta, long currentFrameStartTime) {
            if (!mContinueAnimation) {
                return false;
            }

            
            float progress = mInterpolator.getInterpolation(
                    Math.min(1.0f, (System.nanoTime() - getStartTime())
                                    / (float)MARGIN_ANIMATION_DURATION));

            
            synchronized (mTarget.getLock()) {
                ImmutableViewportMetrics oldMetrics = mTarget.getViewportMetrics();
                ImmutableViewportMetrics newMetrics = oldMetrics.setMargins(
                        FloatUtils.interpolate(mStartLeft, mLeft, progress),
                        FloatUtils.interpolate(mStartTop, mTop, progress),
                        FloatUtils.interpolate(mStartRight, mRight, progress),
                        FloatUtils.interpolate(mStartBottom, mBottom, progress));
                PointF oldOffset = oldMetrics.getMarginOffset();
                PointF newOffset = newMetrics.getMarginOffset();
                newMetrics =
                        newMetrics.offsetViewportByAndClamp(newOffset.x - oldOffset.x,
                                                            newOffset.y - oldOffset.y);

                if (progress >= 1.0f) {
                    mContinueAnimation = false;

                    
                    mTarget.forceViewportMetrics(newMetrics, true, true);
                } else {
                    mTarget.forceViewportMetrics(newMetrics, false, false);
                }
            }
            return mContinueAnimation;
        }
    }

}
