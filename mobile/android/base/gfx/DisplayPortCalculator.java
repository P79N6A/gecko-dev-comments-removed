




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.util.FloatUtils;

import org.json.JSONArray;

import android.graphics.PointF;
import android.graphics.RectF;
import android.util.FloatMath;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

final class DisplayPortCalculator {
    private static final String LOGTAG = "GeckoDisplayPort";
    private static final PointF ZERO_VELOCITY = new PointF(0, 0);

    
    private static final int TILE_SIZE = 256;

    private static final String PREF_DISPLAYPORT_STRATEGY = "gfx.displayport.strategy";
    private static final String PREF_DISPLAYPORT_FM_MULTIPLIER = "gfx.displayport.strategy_fm.multiplier";
    private static final String PREF_DISPLAYPORT_FM_DANGER_X = "gfx.displayport.strategy_fm.danger_x";
    private static final String PREF_DISPLAYPORT_FM_DANGER_Y = "gfx.displayport.strategy_fm.danger_y";
    private static final String PREF_DISPLAYPORT_VB_MULTIPLIER = "gfx.displayport.strategy_vb.multiplier";
    private static final String PREF_DISPLAYPORT_VB_VELOCITY_THRESHOLD = "gfx.displayport.strategy_vb.threshold";
    private static final String PREF_DISPLAYPORT_VB_REVERSE_BUFFER = "gfx.displayport.strategy_vb.reverse_buffer";
    private static final String PREF_DISPLAYPORT_VB_DANGER_X_BASE = "gfx.displayport.strategy_vb.danger_x_base";
    private static final String PREF_DISPLAYPORT_VB_DANGER_Y_BASE = "gfx.displayport.strategy_vb.danger_y_base";
    private static final String PREF_DISPLAYPORT_VB_DANGER_X_INCR = "gfx.displayport.strategy_vb.danger_x_incr";
    private static final String PREF_DISPLAYPORT_VB_DANGER_Y_INCR = "gfx.displayport.strategy_vb.danger_y_incr";
    private static final String PREF_DISPLAYPORT_PB_VELOCITY_THRESHOLD = "gfx.displayport.strategy_pb.threshold";

    private static DisplayPortStrategy sStrategy = new VelocityBiasStrategy(null);

    static DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
        return sStrategy.calculate(metrics, (velocity == null ? ZERO_VELOCITY : velocity));
    }

    static boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
        if (displayPort == null) {
            return true;
        }
        return sStrategy.aboutToCheckerboard(metrics, (velocity == null ? ZERO_VELOCITY : velocity), displayPort);
    }

    static boolean drawTimeUpdate(long millis, int pixels) {
        return sStrategy.drawTimeUpdate(millis, pixels);
    }

    static void resetPageState() {
        sStrategy.resetPageState();
    }

    static void initPrefs() {
        JSONArray prefs = new JSONArray();
        prefs.put(PREF_DISPLAYPORT_STRATEGY);
        prefs.put(PREF_DISPLAYPORT_FM_MULTIPLIER);
        prefs.put(PREF_DISPLAYPORT_FM_DANGER_X);
        prefs.put(PREF_DISPLAYPORT_FM_DANGER_Y);
        prefs.put(PREF_DISPLAYPORT_VB_MULTIPLIER);
        prefs.put(PREF_DISPLAYPORT_VB_VELOCITY_THRESHOLD);
        prefs.put(PREF_DISPLAYPORT_VB_REVERSE_BUFFER);
        prefs.put(PREF_DISPLAYPORT_VB_DANGER_X_BASE);
        prefs.put(PREF_DISPLAYPORT_VB_DANGER_Y_BASE);
        prefs.put(PREF_DISPLAYPORT_VB_DANGER_X_INCR);
        prefs.put(PREF_DISPLAYPORT_VB_DANGER_Y_INCR);
        prefs.put(PREF_DISPLAYPORT_PB_VELOCITY_THRESHOLD);

        PrefsHelper.getPrefs(prefs, new PrefsHelper.PrefHandlerBase() {
            private Map<String, Integer> mValues = new HashMap<String, Integer>();

            @Override public void prefValue(String pref, int value) {
                mValues.put(pref, value);
            }

            @Override public void finish() {
                setStrategy(mValues);
            }
        });
    }

    




    static boolean setStrategy(Map<String, Integer> prefs) {
        Integer strategy = prefs.get(PREF_DISPLAYPORT_STRATEGY);
        if (strategy == null) {
            return false;
        }

        switch (strategy) {
            case 0:
                sStrategy = new FixedMarginStrategy(prefs);
                break;
            case 1:
                sStrategy = new VelocityBiasStrategy(prefs);
                break;
            case 2:
                sStrategy = new DynamicResolutionStrategy(prefs);
                break;
            case 3:
                sStrategy = new NoMarginStrategy(prefs);
                break;
            case 4:
                sStrategy = new PredictionBiasStrategy(prefs);
                break;
            default:
                Log.e(LOGTAG, "Invalid strategy index specified");
                return false;
        }
        Log.i(LOGTAG, "Set strategy " + sStrategy.toString());
        return true;
    }

    private static float getFloatPref(Map<String, Integer> prefs, String prefName, int defaultValue) {
        Integer value = (prefs == null ? null : prefs.get(prefName));
        return (float)(value == null || value < 0 ? defaultValue : value) / 1000f;
    }

    private static abstract class DisplayPortStrategy {
        
        public abstract DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity);
        
        public abstract boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort);
        
        public boolean drawTimeUpdate(long millis, int pixels) { return false; }
        
        public void resetPageState() {}
    }

    





    private static FloatSize reshapeForPage(float width, float height, ImmutableViewportMetrics metrics) {
        
        float usableWidth = Math.min(width, metrics.getPageWidth());
        
        
        float extraUsableHeight = (float)Math.floor(((width - usableWidth) * height) / usableWidth);
        float usableHeight = Math.min(height + extraUsableHeight, metrics.getPageHeight());
        if (usableHeight < height && usableWidth == width) {
            
            float extraUsableWidth = (float)Math.floor(((height - usableHeight) * width) / usableHeight);
            usableWidth = Math.min(width + extraUsableWidth, metrics.getPageWidth());
        }
        return new FloatSize(usableWidth, usableHeight);
    }

    




    private static RectF expandByDangerZone(RectF rect, float dangerZoneXMultiplier, float dangerZoneYMultiplier, ImmutableViewportMetrics metrics) {
        
        float dangerZoneX = metrics.getWidth() * dangerZoneXMultiplier;
        float dangerZoneY = metrics.getHeight() * dangerZoneYMultiplier;
        rect = RectUtils.expand(rect, dangerZoneX, dangerZoneY);
        
        return clampToPageBounds(rect, metrics);
    }

    





    private static DisplayPortMetrics getTileAlignedDisplayPortMetrics(RectF margins, float zoom, ImmutableViewportMetrics metrics) {
        float left = metrics.viewportRectLeft - margins.left;
        float top = metrics.viewportRectTop - margins.top;
        float right = metrics.viewportRectRight + margins.right;
        float bottom = metrics.viewportRectBottom + margins.bottom;
        left = Math.max(metrics.pageRectLeft, TILE_SIZE * FloatMath.floor(left / TILE_SIZE));
        top = Math.max(metrics.pageRectTop, TILE_SIZE * FloatMath.floor(top / TILE_SIZE));
        right = Math.min(metrics.pageRectRight, TILE_SIZE * FloatMath.ceil(right / TILE_SIZE));
        bottom = Math.min(metrics.pageRectBottom, TILE_SIZE * FloatMath.ceil(bottom / TILE_SIZE));
        return new DisplayPortMetrics(left, top, right, bottom, zoom);
    }

    





    private static RectF shiftMarginsForPageBounds(RectF margins, ImmutableViewportMetrics metrics) {
        
        
        
        float leftOverflow = metrics.pageRectLeft - (metrics.viewportRectLeft - margins.left);
        float rightOverflow = (metrics.viewportRectRight + margins.right) - metrics.pageRectRight;
        float topOverflow = metrics.pageRectTop - (metrics.viewportRectTop - margins.top);
        float bottomOverflow = (metrics.viewportRectBottom + margins.bottom) - metrics.pageRectBottom;

        
        if (leftOverflow > 0) {
            margins.left -= leftOverflow;
            margins.right += leftOverflow;
        } else if (rightOverflow > 0) {
            margins.right -= rightOverflow;
            margins.left += rightOverflow;
        }
        if (topOverflow > 0) {
            margins.top -= topOverflow;
            margins.bottom += topOverflow;
        } else if (bottomOverflow > 0) {
            margins.bottom -= bottomOverflow;
            margins.top += bottomOverflow;
        }
        return margins;
    }

    


    private static RectF clampToPageBounds(RectF rect, ImmutableViewportMetrics metrics) {
        if (rect.top < metrics.pageRectTop) rect.top = metrics.pageRectTop;
        if (rect.left < metrics.pageRectLeft) rect.left = metrics.pageRectLeft;
        if (rect.right > metrics.pageRectRight) rect.right = metrics.pageRectRight;
        if (rect.bottom > metrics.pageRectBottom) rect.bottom = metrics.pageRectBottom;
        return rect;
    }

    


    private static class NoMarginStrategy extends DisplayPortStrategy {
        NoMarginStrategy(Map<String, Integer> prefs) {
            
        }

        @Override
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            return new DisplayPortMetrics(metrics.viewportRectLeft,
                    metrics.viewportRectTop,
                    metrics.viewportRectRight,
                    metrics.viewportRectBottom,
                    metrics.zoomFactor);
        }

        @Override
        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            return true;
        }

        @Override
        public String toString() {
            return "NoMarginStrategy";
        }
    }

    







    private static class FixedMarginStrategy extends DisplayPortStrategy {
        
        
        private final float SIZE_MULTIPLIER;

        
        
        private final float DANGER_ZONE_X_MULTIPLIER;
        private final float DANGER_ZONE_Y_MULTIPLIER;

        FixedMarginStrategy(Map<String, Integer> prefs) {
            SIZE_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_FM_MULTIPLIER, 2000);
            DANGER_ZONE_X_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_FM_DANGER_X, 100);
            DANGER_ZONE_Y_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_FM_DANGER_Y, 200);
        }

        @Override
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            float displayPortWidth = metrics.getWidth() * SIZE_MULTIPLIER;
            float displayPortHeight = metrics.getHeight() * SIZE_MULTIPLIER;

            
            
            
            
            FloatSize usableSize = reshapeForPage(displayPortWidth, displayPortHeight, metrics);
            float horizontalBuffer = usableSize.width - metrics.getWidth();
            float verticalBuffer = usableSize.height - metrics.getHeight();

            
            
            
            
            
            RectF margins = new RectF();
            margins.left = horizontalBuffer / 2.0f;
            margins.right = horizontalBuffer - margins.left;
            margins.top = verticalBuffer / 2.0f;
            margins.bottom = verticalBuffer - margins.top;
            margins = shiftMarginsForPageBounds(margins, metrics);

            return getTileAlignedDisplayPortMetrics(margins, metrics.zoomFactor, metrics);
        }

        @Override
        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            
            
            
            RectF adjustedViewport = expandByDangerZone(metrics.getViewport(), DANGER_ZONE_X_MULTIPLIER, DANGER_ZONE_Y_MULTIPLIER, metrics);
            return !displayPort.contains(adjustedViewport);
        }

        @Override
        public String toString() {
            return "FixedMarginStrategy mult=" + SIZE_MULTIPLIER + ", dangerX=" + DANGER_ZONE_X_MULTIPLIER + ", dangerY=" + DANGER_ZONE_Y_MULTIPLIER;
        }
    }

    








    private static class VelocityBiasStrategy extends DisplayPortStrategy {
        
        
        private final float SIZE_MULTIPLIER;
        
        private final float VELOCITY_THRESHOLD;
        
        private final float REVERSE_BUFFER;
        
        
        
        
        private final float DANGER_ZONE_BASE_X_MULTIPLIER;
        private final float DANGER_ZONE_BASE_Y_MULTIPLIER;
        private final float DANGER_ZONE_INCR_X_MULTIPLIER;
        private final float DANGER_ZONE_INCR_Y_MULTIPLIER;

        VelocityBiasStrategy(Map<String, Integer> prefs) {
            SIZE_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_VB_MULTIPLIER, 2000);
            VELOCITY_THRESHOLD = GeckoAppShell.getDpi() * getFloatPref(prefs, PREF_DISPLAYPORT_VB_VELOCITY_THRESHOLD, 32);
            REVERSE_BUFFER = getFloatPref(prefs, PREF_DISPLAYPORT_VB_REVERSE_BUFFER, 200);
            DANGER_ZONE_BASE_X_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_VB_DANGER_X_BASE, 1000);
            DANGER_ZONE_BASE_Y_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_VB_DANGER_Y_BASE, 1000);
            DANGER_ZONE_INCR_X_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_VB_DANGER_X_INCR, 0);
            DANGER_ZONE_INCR_Y_MULTIPLIER = getFloatPref(prefs, PREF_DISPLAYPORT_VB_DANGER_Y_INCR, 0);
        }

        







        private RectF velocityBiasedMargins(float xAmount, float yAmount, PointF velocity) {
            RectF margins = new RectF();

            if (velocity.x > VELOCITY_THRESHOLD) {
                margins.left = xAmount * REVERSE_BUFFER;
            } else if (velocity.x < -VELOCITY_THRESHOLD) {
                margins.left = xAmount * (1.0f - REVERSE_BUFFER);
            } else {
                margins.left = xAmount / 2.0f;
            }
            margins.right = xAmount - margins.left;
    
            if (velocity.y > VELOCITY_THRESHOLD) {
                margins.top = yAmount * REVERSE_BUFFER;
            } else if (velocity.y < -VELOCITY_THRESHOLD) {
                margins.top = yAmount * (1.0f - REVERSE_BUFFER);
            } else {
                margins.top = yAmount / 2.0f;
            }
            margins.bottom = yAmount - margins.top;

            return margins;
        }

        @Override
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            float displayPortWidth = metrics.getWidth() * SIZE_MULTIPLIER;
            float displayPortHeight = metrics.getHeight() * SIZE_MULTIPLIER;

            
            
            if (Math.abs(velocity.x) > VELOCITY_THRESHOLD && FloatUtils.fuzzyEquals(velocity.y, 0)) {
                displayPortHeight = metrics.getHeight();
            } else if (Math.abs(velocity.y) > VELOCITY_THRESHOLD && FloatUtils.fuzzyEquals(velocity.x, 0)) {
                displayPortWidth = metrics.getWidth();
            }

            
            
            displayPortWidth = Math.min(displayPortWidth, metrics.getPageWidth());
            displayPortHeight = Math.min(displayPortHeight, metrics.getPageHeight());
            float horizontalBuffer = displayPortWidth - metrics.getWidth();
            float verticalBuffer = displayPortHeight - metrics.getHeight();

            
            
            RectF margins = velocityBiasedMargins(horizontalBuffer, verticalBuffer, velocity);
            margins = shiftMarginsForPageBounds(margins, metrics);

            return getTileAlignedDisplayPortMetrics(margins, metrics.zoomFactor, metrics);
        }

        @Override
        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            
            float dangerZoneX = metrics.getWidth() * (DANGER_ZONE_BASE_X_MULTIPLIER + (velocity.x * DANGER_ZONE_INCR_X_MULTIPLIER));
            float dangerZoneY = metrics.getHeight() * (DANGER_ZONE_BASE_Y_MULTIPLIER + (velocity.y * DANGER_ZONE_INCR_Y_MULTIPLIER));
            
            
            dangerZoneX = Math.min(dangerZoneX, metrics.getPageWidth() - metrics.getWidth());
            dangerZoneY = Math.min(dangerZoneY, metrics.getPageHeight() - metrics.getHeight());

            
            
            RectF dangerMargins = velocityBiasedMargins(dangerZoneX, dangerZoneY, velocity);
            dangerMargins = shiftMarginsForPageBounds(dangerMargins, metrics);

            
            
            RectF adjustedViewport = new RectF(
                    metrics.viewportRectLeft - dangerMargins.left,
                    metrics.viewportRectTop - dangerMargins.top,
                    metrics.viewportRectRight + dangerMargins.right,
                    metrics.viewportRectBottom + dangerMargins.bottom);
            return !displayPort.contains(adjustedViewport);
        }

        @Override
        public String toString() {
            return "VelocityBiasStrategy mult=" + SIZE_MULTIPLIER + ", threshold=" + VELOCITY_THRESHOLD + ", reverse=" + REVERSE_BUFFER
                + ", dangerBaseX=" + DANGER_ZONE_BASE_X_MULTIPLIER + ", dangerBaseY=" + DANGER_ZONE_BASE_Y_MULTIPLIER
                + ", dangerIncrX=" + DANGER_ZONE_INCR_Y_MULTIPLIER + ", dangerIncrY=" + DANGER_ZONE_INCR_Y_MULTIPLIER;
        }
    }

    







    private static class DynamicResolutionStrategy extends DisplayPortStrategy {
        
        
        private static final float SIZE_MULTIPLIER = 1.5f;

        
        
        private static final float VELOCITY_EXPANSION_THRESHOLD = GeckoAppShell.getDpi() / 16f;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        private static final float VELOCITY_MULTIPLIER = 60.0f;

        
        
        
        
        
        
        private static final float VELOCITY_FAST_THRESHOLD = VELOCITY_EXPANSION_THRESHOLD * 2.0f;
        private static final float FAST_SPLIT_FACTOR = 0.95f;
        private static final float SLOW_SPLIT_FACTOR = 0.8f;

        
        
        
        
        
        
        
        
        
        
        
        
        private static final float PREDICTION_VELOCITY_MULTIPLIER = 30.0f;
        private static final float DANGER_ZONE_MULTIPLIER = 0.20f; 

        DynamicResolutionStrategy(Map<String, Integer> prefs) {
            
        }

        @Override
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            float displayPortWidth = metrics.getWidth() * SIZE_MULTIPLIER;
            float displayPortHeight = metrics.getHeight() * SIZE_MULTIPLIER;

            
            
            
            FloatSize reshapedSize = reshapeForPage(displayPortWidth, displayPortHeight, metrics);

            
            
            if (velocity.length() > VELOCITY_EXPANSION_THRESHOLD) {
                float velocityFactor = Math.max(Math.abs(velocity.x) / displayPortWidth,
                                                Math.abs(velocity.y) / displayPortHeight);
                velocityFactor *= VELOCITY_MULTIPLIER;

                displayPortWidth += (displayPortWidth * velocityFactor);
                displayPortHeight += (displayPortHeight * velocityFactor);
            }

            
            
            

            
            
            
            
            
            
            FloatSize usableSize = reshapeForPage(displayPortWidth, displayPortHeight, metrics);
            float horizontalBuffer = usableSize.width - metrics.getWidth();
            float verticalBuffer = usableSize.height - metrics.getHeight();

            
            
            
            
            
            RectF margins = new RectF();
            margins.left = splitBufferByVelocity(horizontalBuffer, velocity.x);
            margins.right = horizontalBuffer - margins.left;
            margins.top = splitBufferByVelocity(verticalBuffer, velocity.y);
            margins.bottom = verticalBuffer - margins.top;

            
            
            margins = shiftMarginsForPageBounds(margins, metrics);

            
            
            
            
            
            
            
            
            
            
            float scaleFactor = Math.min(reshapedSize.width / usableSize.width, reshapedSize.height / usableSize.height);
            float displayResolution = metrics.zoomFactor * Math.min(1.0f, scaleFactor);

            DisplayPortMetrics dpMetrics = new DisplayPortMetrics(
                metrics.viewportRectLeft - margins.left,
                metrics.viewportRectTop - margins.top,
                metrics.viewportRectRight + margins.right,
                metrics.viewportRectBottom + margins.bottom,
                displayResolution);
            return dpMetrics;
        }

        






        private float splitBufferByVelocity(float amount, float velocity) {
            
            if (FloatUtils.fuzzyEquals(velocity, 0)) {
                return amount / 2.0f;
            }
            
            
            if (velocity < -VELOCITY_FAST_THRESHOLD) {
                return amount * FAST_SPLIT_FACTOR;
            }
            if (velocity > VELOCITY_FAST_THRESHOLD) {
                return amount * (1.0f - FAST_SPLIT_FACTOR);
            }
            
            if (velocity < 0) {
                return amount * SLOW_SPLIT_FACTOR;
            } else {
                return amount * (1.0f - SLOW_SPLIT_FACTOR);
            }
        }

        @Override
        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            
            
            

            RectF predictedViewport = metrics.getViewport();

            
            
            if (velocity.length() > 0) {
                if (velocity.x < 0) {
                    predictedViewport.left += velocity.x * PREDICTION_VELOCITY_MULTIPLIER;
                } else if (velocity.x > 0) {
                    predictedViewport.right += velocity.x * PREDICTION_VELOCITY_MULTIPLIER;
                }

                if (velocity.y < 0) {
                    predictedViewport.top += velocity.y * PREDICTION_VELOCITY_MULTIPLIER;
                } else if (velocity.y > 0) {
                    predictedViewport.bottom += velocity.y * PREDICTION_VELOCITY_MULTIPLIER;
                }
            }

            
            
            predictedViewport = expandByDangerZone(predictedViewport, DANGER_ZONE_MULTIPLIER, DANGER_ZONE_MULTIPLIER, metrics);
            return !displayPort.contains(predictedViewport);
        }

        @Override
        public String toString() {
            return "DynamicResolutionStrategy";
        }
    }

    









    private static class PredictionBiasStrategy extends DisplayPortStrategy {
        private static float VELOCITY_THRESHOLD;

        private int mPixelArea;         
        private int mMinFramesToDraw;   
        private int mMaxFramesToDraw;   

        PredictionBiasStrategy(Map<String, Integer> prefs) {
            VELOCITY_THRESHOLD = GeckoAppShell.getDpi() * getFloatPref(prefs, PREF_DISPLAYPORT_PB_VELOCITY_THRESHOLD, 16);
            resetPageState();
        }

        @Override
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            float width = metrics.getWidth();
            float height = metrics.getHeight();
            mPixelArea = (int)(width * height);

            if (velocity.length() < VELOCITY_THRESHOLD) {
                
                RectF margins = new RectF(width, height, width, height);
                return getTileAlignedDisplayPortMetrics(margins, metrics.zoomFactor, metrics);
            }

            
            float minDx = velocity.x * mMinFramesToDraw;
            float minDy = velocity.y * mMinFramesToDraw;
            float maxDx = velocity.x * mMaxFramesToDraw;
            float maxDy = velocity.y * mMaxFramesToDraw;

            
            
            float pixelsToDraw = (width + Math.abs(maxDx - minDx)) * (height + Math.abs(maxDy - minDy));
            
            
            
            maxDx = maxDx * pixelsToDraw / mPixelArea;
            maxDy = maxDy * pixelsToDraw / mPixelArea;

            
            
            RectF margins = new RectF(
                -Math.min(minDx, maxDx),
                -Math.min(minDy, maxDy),
                Math.max(minDx, maxDx),
                Math.max(minDy, maxDy));
            return getTileAlignedDisplayPortMetrics(margins, metrics.zoomFactor, metrics);
        }

        @Override
        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            
            
            float minDx = velocity.x * mMinFramesToDraw;
            float minDy = velocity.y * mMinFramesToDraw;
            float maxDx = velocity.x * mMaxFramesToDraw;
            float maxDy = velocity.y * mMaxFramesToDraw;
            float pixelsToDraw = (metrics.getWidth() + Math.abs(maxDx - minDx)) * (metrics.getHeight() + Math.abs(maxDy - minDy));
            maxDx = maxDx * pixelsToDraw / mPixelArea;
            maxDy = maxDy * pixelsToDraw / mPixelArea;

            
            
            
            RectF predictedViewport = metrics.getViewport();
            predictedViewport.left += maxDx;
            predictedViewport.top += maxDy;
            predictedViewport.right += maxDx;
            predictedViewport.bottom += maxDy;

            predictedViewport = clampToPageBounds(predictedViewport, metrics);
            return !displayPort.contains(predictedViewport);
        }

        @Override
        public boolean drawTimeUpdate(long millis, int pixels) {
            
            float normalizedTime = (float)mPixelArea * (float)millis / (float)pixels;
            int normalizedFrames = (int)FloatMath.ceil(normalizedTime * 60f / 1000f);
            
            
            
            if (normalizedFrames <= mMinFramesToDraw) {
                mMinFramesToDraw--;
            } else if (normalizedFrames > mMaxFramesToDraw) {
                mMaxFramesToDraw++;
            } else {
                return true;
            }
            Log.d(LOGTAG, "Widened draw range to [" + mMinFramesToDraw + ", " + mMaxFramesToDraw + "]");
            return true;
        }

        @Override
        public void resetPageState() {
            mMinFramesToDraw = 0;
            mMaxFramesToDraw = 2;
        }

        @Override
        public String toString() {
            return "PredictionBiasStrategy threshold=" + VELOCITY_THRESHOLD;
        }
    }
}
