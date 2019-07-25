




package org.mozilla.gecko.gfx;

import android.graphics.PointF;
import android.graphics.RectF;
import android.util.Log;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoAppShell;

final class DisplayPortCalculator {
    private static final String LOGTAG = "GeckoDisplayPortCalculator";
    private static final PointF ZERO_VELOCITY = new PointF(0, 0);

    private static DisplayPortStrategy sStrategy = new FixedMarginStrategy();

    static DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
        return sStrategy.calculate(metrics, (velocity == null ? ZERO_VELOCITY : velocity));
    }

    static boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
        if (displayPort == null) {
            return true;
        }
        return sStrategy.aboutToCheckerboard(metrics, (velocity == null ? ZERO_VELOCITY : velocity), displayPort);
    }

    




    static void setStrategy(int strategy) {
        switch (strategy) {
            case 0:
            default:
                sStrategy = new FixedMarginStrategy();
                break;
            case 1:
                sStrategy = new VelocityBiasStrategy();
                break;
            case 2:
                sStrategy = new DynamicResolutionStrategy();
                break;
            case 3:
                sStrategy = new NoMarginStrategy();
                break;
        }
        Log.i(LOGTAG, "Set strategy " + sStrategy.getClass().getName());
    }

    private interface DisplayPortStrategy {
        
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity);
        
        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort);
    }

    





    private static FloatSize reshapeForPage(float width, float height, ImmutableViewportMetrics metrics) {
        
        float usableWidth = Math.min(width, metrics.pageSizeWidth);
        
        
        float extraUsableHeight = (float)Math.floor(((width - usableWidth) * height) / usableWidth);
        float usableHeight = Math.min(height + extraUsableHeight, metrics.pageSizeHeight);
        if (usableHeight < height && usableWidth == width) {
            
            float extraUsableWidth = (float)Math.floor(((height - usableHeight) * width) / usableHeight);
            usableWidth = Math.min(width + extraUsableWidth, metrics.pageSizeWidth);
        }
        return new FloatSize(usableWidth, usableHeight);
    }

    




    private static RectF expandByDangerZone(RectF rect, float dangerZoneXMultiplier, float dangerZoneYMultiplier, ImmutableViewportMetrics metrics) {
        
        float dangerZoneX = metrics.getWidth() * dangerZoneXMultiplier;
        float dangerZoneY = metrics.getHeight() * dangerZoneYMultiplier;
        rect = RectUtils.expand(rect, dangerZoneX, dangerZoneY);
        
        if (rect.top < 0) rect.top = 0;
        if (rect.left < 0) rect.left = 0;
        if (rect.right > metrics.pageSizeWidth) rect.right = metrics.pageSizeWidth;
        if (rect.bottom > metrics.pageSizeHeight) rect.bottom = metrics.pageSizeHeight;
        return rect;
    }

    





    private static RectF shiftMarginsForPageBounds(RectF margins, ImmutableViewportMetrics metrics) {
        
        
        
        float leftOverflow = margins.left - metrics.viewportRectLeft;
        float rightOverflow = margins.right - (metrics.pageSizeWidth - metrics.viewportRectRight);
        float topOverflow = margins.top - metrics.viewportRectTop;
        float bottomOverflow = margins.bottom - (metrics.pageSizeHeight - metrics.viewportRectBottom);

        
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

    


    private static class NoMarginStrategy implements DisplayPortStrategy {
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            return new DisplayPortMetrics(metrics.viewportRectLeft,
                    metrics.viewportRectTop,
                    metrics.viewportRectRight,
                    metrics.viewportRectBottom,
                    metrics.zoomFactor);
        }

        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            return true;
        }
    }

    







    private static class FixedMarginStrategy implements DisplayPortStrategy {
        
        
        private static final float SIZE_MULTIPLIER = 1.5f;

        
        
        private static final float DANGER_ZONE_X_MULTIPLIER = 0.10f;
        private static final float DANGER_ZONE_Y_MULTIPLIER = 0.20f;

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

            
            
            
            
            return new DisplayPortMetrics(metrics.viewportRectLeft - margins.left,
                    metrics.viewportRectTop - margins.top,
                    metrics.viewportRectRight + margins.right,
                    metrics.viewportRectBottom + margins.bottom,
                    metrics.zoomFactor);
        }

        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            
            
            
            RectF adjustedViewport = expandByDangerZone(metrics.getViewport(), DANGER_ZONE_X_MULTIPLIER, DANGER_ZONE_Y_MULTIPLIER, metrics);
            return !displayPort.contains(adjustedViewport);
        }
    }

    








    private static class VelocityBiasStrategy implements DisplayPortStrategy {
        
        
        private static final float SIZE_MULTIPLIER = 1.5f;
        
        private static final float VELOCITY_THRESHOLD = GeckoAppShell.getDpi() / 32f;
        
        private static final float REVERSE_BUFFER = 0.2f;

        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            float displayPortWidth = metrics.getWidth() * SIZE_MULTIPLIER;
            float displayPortHeight = metrics.getHeight() * SIZE_MULTIPLIER;

            
            
            if (Math.abs(velocity.x) > VELOCITY_THRESHOLD && FloatUtils.fuzzyEquals(velocity.y, 0)) {
                displayPortHeight = metrics.getHeight();
            } else if (Math.abs(velocity.y) > VELOCITY_THRESHOLD && FloatUtils.fuzzyEquals(velocity.x, 0)) {
                displayPortWidth = metrics.getWidth();
            }

            
            
            displayPortWidth = Math.min(displayPortWidth, metrics.pageSizeWidth);
            displayPortHeight = Math.min(displayPortHeight, metrics.pageSizeHeight);
            float horizontalBuffer = displayPortWidth - metrics.getWidth();
            float verticalBuffer = displayPortHeight - metrics.getHeight();

            
            
            
            RectF margins = new RectF();
            if (velocity.x > VELOCITY_THRESHOLD) {
                margins.left = horizontalBuffer * REVERSE_BUFFER;
            } else if (velocity.x < -VELOCITY_THRESHOLD) {
                margins.left = horizontalBuffer * (1.0f - REVERSE_BUFFER);
            } else {
                margins.left = horizontalBuffer / 2.0f;
            }
            margins.right = horizontalBuffer - margins.left;

            if (velocity.y > VELOCITY_THRESHOLD) {
                margins.top = verticalBuffer * REVERSE_BUFFER;
            } else if (velocity.y < -VELOCITY_THRESHOLD) {
                margins.top = verticalBuffer * (1.0f - REVERSE_BUFFER);
            } else {
                margins.top = verticalBuffer / 2.0f;
            }
            margins.bottom = verticalBuffer - margins.top;

            
            margins = shiftMarginsForPageBounds(margins, metrics);

            return new DisplayPortMetrics(metrics.viewportRectLeft - margins.left,
                    metrics.viewportRectTop - margins.top,
                    metrics.viewportRectRight + margins.right,
                    metrics.viewportRectBottom + margins.bottom,
                    metrics.zoomFactor);
        }

        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            
            
            
            
            
            return true;
        }
    }

    







    private static class DynamicResolutionStrategy implements DisplayPortStrategy {
        
        
        private static final float SIZE_MULTIPLIER = 1.5f;

        
        
        private static final float VELOCITY_EXPANSION_THRESHOLD = GeckoAppShell.getDpi() / 16f;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        private static final float VELOCITY_MULTIPLIER = 60.0f;

        
        
        
        
        
        
        private static final float VELOCITY_FAST_THRESHOLD = VELOCITY_EXPANSION_THRESHOLD * 2.0f;
        private static final float FAST_SPLIT_FACTOR = 0.95f;
        private static final float SLOW_SPLIT_FACTOR = 0.8f;

        
        
        
        
        
        
        
        
        
        
        
        
        private static final float PREDICTION_VELOCITY_MULTIPLIER = 30.0f;
        private static final float DANGER_ZONE_MULTIPLIER = 0.20f; 

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
    }
}
