




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.FloatUtils;

import android.graphics.PointF;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.Log;
import android.view.animation.DecelerateInterpolator;

import java.util.Timer;
import java.util.TimerTask;

public class LayerMarginsAnimator {
    private static final String LOGTAG = "GeckoLayerMarginsAnimator";
    private static final float MS_PER_FRAME = 1000.0f / 60.0f;
    private static final long MARGIN_ANIMATION_DURATION = 250;

    
    private final RectF mMaxMargins;
    
    private boolean mMarginsPinned;
    
    private Timer mAnimationTimer;
    
    private final DecelerateInterpolator mInterpolator;
    
    private final GeckoLayerClient mTarget;

    public LayerMarginsAnimator(GeckoLayerClient aTarget) {
        
        mTarget = aTarget;

        
        mMaxMargins = new RectF();
        mInterpolator = new DecelerateInterpolator();
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
                        mAnimationTimer.cancel();
                        mAnimationTimer = null;

                        
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
        mMarginsPinned = pin;
    }

    



    ImmutableViewportMetrics scrollBy(ImmutableViewportMetrics aMetrics, float aDx, float aDy) {
        
        if (mAnimationTimer != null) {
            mAnimationTimer.cancel();
            mAnimationTimer = null;
        }

        float newMarginLeft = aMetrics.marginLeft;
        float newMarginTop = aMetrics.marginTop;
        float newMarginRight = aMetrics.marginRight;
        float newMarginBottom = aMetrics.marginBottom;

        
        if (!mMarginsPinned) {
            RectF overscroll = aMetrics.getOverscroll();
            if (aDx >= 0) {
              
              float marginDx = Math.max(0, aDx - overscroll.left);
              newMarginLeft = aMetrics.marginLeft - Math.min(marginDx, aMetrics.marginLeft);
              newMarginRight = aMetrics.marginRight + Math.min(marginDx, mMaxMargins.right - aMetrics.marginRight);

              aDx -= aMetrics.marginLeft - newMarginLeft;
            } else {
              
              float marginDx = Math.max(0, -aDx - overscroll.right);
              newMarginLeft = aMetrics.marginLeft + Math.min(marginDx, mMaxMargins.left - aMetrics.marginLeft);
              newMarginRight = aMetrics.marginRight - Math.min(marginDx, aMetrics.marginRight);

              aDx -= aMetrics.marginLeft - newMarginLeft;
            }

            if (aDy >= 0) {
              
              float marginDy = Math.max(0, aDy - overscroll.top);
              newMarginTop = aMetrics.marginTop - Math.min(marginDy, aMetrics.marginTop);
              newMarginBottom = aMetrics.marginBottom + Math.min(marginDy, mMaxMargins.bottom - aMetrics.marginBottom);

              aDy -= aMetrics.marginTop - newMarginTop;
            } else {
              
              float marginDy = Math.max(0, -aDy - overscroll.bottom);
              newMarginTop = aMetrics.marginTop + Math.min(marginDy, mMaxMargins.top - aMetrics.marginTop);
              newMarginBottom = aMetrics.marginBottom - Math.min(marginDy, aMetrics.marginBottom);

              aDy -= aMetrics.marginTop - newMarginTop;
            }
        }

        return aMetrics.setMargins(newMarginLeft, newMarginTop, newMarginRight, newMarginBottom).offsetViewportBy(aDx, aDy);
    }
}
