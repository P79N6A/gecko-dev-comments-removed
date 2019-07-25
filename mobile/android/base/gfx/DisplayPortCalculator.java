




package org.mozilla.gecko.gfx;

import android.graphics.PointF;
import android.graphics.RectF;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoAppShell;

final class DisplayPortCalculator {
    private static final String LOGTAG = "GeckoDisplayPortCalculator";

    private static DisplayPortStrategy sStrategy = new FixedMarginStrategy();

    static DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
        return sStrategy.calculate(metrics, velocity);
    }

    static boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
        return sStrategy.aboutToCheckerboard(metrics, velocity, displayPort);
    }

    private interface DisplayPortStrategy {
        
        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity);
        
        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort);
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
        private static final int DEFAULT_DISPLAY_PORT_MARGIN = 300;

        

        private static final int DANGER_ZONE_X = 75;
        private static final int DANGER_ZONE_Y = 150;

        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            float desiredXMargins = 2 * DEFAULT_DISPLAY_PORT_MARGIN;
            float desiredYMargins = 2 * DEFAULT_DISPLAY_PORT_MARGIN;

            
            
            

            
            float xBufferAmount = Math.min(desiredXMargins, metrics.pageSizeWidth - metrics.getWidth());
            
            
            float savedPixels = (desiredXMargins - xBufferAmount) * (metrics.getHeight() + desiredYMargins);
            float extraYAmount = (float)Math.floor(savedPixels / (metrics.getWidth() + xBufferAmount));
            float yBufferAmount = Math.min(desiredYMargins + extraYAmount, metrics.pageSizeHeight - metrics.getHeight());
            
            if (xBufferAmount == desiredXMargins && yBufferAmount < desiredYMargins) {
                savedPixels = (desiredYMargins - yBufferAmount) * (metrics.getWidth() + xBufferAmount);
                float extraXAmount = (float)Math.floor(savedPixels / (metrics.getHeight() + yBufferAmount));
                xBufferAmount = Math.min(xBufferAmount + extraXAmount, metrics.pageSizeWidth - metrics.getWidth());
            }

            
            
            
            
            
            float leftMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.viewportRectLeft);
            float rightMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.pageSizeWidth - (metrics.viewportRectLeft + metrics.getWidth()));
            if (leftMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
                rightMargin = xBufferAmount - leftMargin;
            } else if (rightMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
                leftMargin = xBufferAmount - rightMargin;
            } else if (!FloatUtils.fuzzyEquals(leftMargin + rightMargin, xBufferAmount)) {
                float delta = xBufferAmount - leftMargin - rightMargin;
                leftMargin += delta / 2;
                rightMargin += delta / 2;
            }

            float topMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.viewportRectTop);
            float bottomMargin = Math.min(DEFAULT_DISPLAY_PORT_MARGIN, metrics.pageSizeHeight - (metrics.viewportRectTop + metrics.getHeight()));
            if (topMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
                bottomMargin = yBufferAmount - topMargin;
            } else if (bottomMargin < DEFAULT_DISPLAY_PORT_MARGIN) {
                topMargin = yBufferAmount - bottomMargin;
            } else if (!FloatUtils.fuzzyEquals(topMargin + bottomMargin, yBufferAmount)) {
                float delta = yBufferAmount - topMargin - bottomMargin;
                topMargin += delta / 2;
                bottomMargin += delta / 2;
            }

            
            
            
            
            return new DisplayPortMetrics(metrics.viewportRectLeft - leftMargin,
                    metrics.viewportRectTop - topMargin,
                    metrics.viewportRectRight + rightMargin,
                    metrics.viewportRectBottom + bottomMargin,
                    metrics.zoomFactor);
        }

        public boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, PointF velocity, DisplayPortMetrics displayPort) {
            if (displayPort == null) {
                return true;
            }

            
            
            
            FloatSize pageSize = metrics.getPageSize();
            RectF adjustedViewport = RectUtils.expand(metrics.getViewport(), DANGER_ZONE_X, DANGER_ZONE_Y);
            if (adjustedViewport.top < 0) adjustedViewport.top = 0;
            if (adjustedViewport.left < 0) adjustedViewport.left = 0;
            if (adjustedViewport.right > pageSize.width) adjustedViewport.right = pageSize.width;
            if (adjustedViewport.bottom > pageSize.height) adjustedViewport.bottom = pageSize.height;

            return !displayPort.contains(adjustedViewport);
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
        private static final float DANGER_ZONE_MULTIPLIER = 0.10f; 

        public DisplayPortMetrics calculate(ImmutableViewportMetrics metrics, PointF velocity) {
            float baseWidth = metrics.getWidth() * SIZE_MULTIPLIER;
            float baseHeight = metrics.getHeight() * SIZE_MULTIPLIER;

            float width = baseWidth;
            float height = baseHeight;
            if (velocity != null && velocity.length() > VELOCITY_EXPANSION_THRESHOLD) {
                
                float velocityFactor = Math.max(Math.abs(velocity.x) / width,
                                                Math.abs(velocity.y) / height);
                velocityFactor *= VELOCITY_MULTIPLIER;

                width += (width * velocityFactor);
                height += (height * velocityFactor);
            }

            
            
            

            
            
            
            
            
            
            
            
            
            
            
            float usableWidth = Math.min(width, metrics.pageSizeWidth);
            float extraUsableHeight = ((width - usableWidth) * height) / usableWidth;
            float usableHeight = Math.min(height + extraUsableHeight, metrics.pageSizeHeight);
            if (usableHeight < height && usableWidth == width) {
                float extraUsableWidth = ((height - usableHeight) * width) / usableHeight;
                usableWidth = Math.min(width + extraUsableWidth, metrics.pageSizeWidth);
            }

            
            
            

            float horizontalBuffer = usableWidth - metrics.getWidth();
            float verticalBuffer = usableHeight - metrics.getHeight();
            
            
            float leftMargin = splitBufferByVelocity(horizontalBuffer, velocity.x);
            float rightMargin = horizontalBuffer - leftMargin;
            float topMargin = splitBufferByVelocity(verticalBuffer, velocity.y);
            float bottomMargin = verticalBuffer - topMargin;

            
            
            if (metrics.viewportRectLeft - leftMargin < 0) {
                leftMargin = metrics.viewportRectLeft;
                rightMargin = horizontalBuffer - leftMargin;
            } else if (metrics.viewportRectRight + rightMargin > metrics.pageSizeWidth) {
                rightMargin = metrics.pageSizeWidth - metrics.viewportRectRight;
                leftMargin = horizontalBuffer - rightMargin;
            }
            if (metrics.viewportRectTop - topMargin < 0) {
                topMargin = metrics.viewportRectTop;
                bottomMargin = verticalBuffer - topMargin;
            } else if (metrics.viewportRectBottom + bottomMargin > metrics.pageSizeHeight) {
                bottomMargin = metrics.pageSizeHeight - metrics.viewportRectBottom;
                topMargin = verticalBuffer - bottomMargin;
            }

            
            
            
            
            
            
            
            
            
            
            float scaleFactor = Math.min(baseWidth / usableWidth, baseHeight / usableHeight);
            float displayResolution = metrics.zoomFactor * Math.min(1.0f, scaleFactor);

            DisplayPortMetrics dpMetrics = new DisplayPortMetrics(
                metrics.viewportRectLeft - leftMargin,
                metrics.viewportRectTop - topMargin,
                metrics.viewportRectRight + rightMargin,
                metrics.viewportRectBottom + bottomMargin,
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
            if (displayPort == null) {
                return true;
            }

            
            
            

            float left = metrics.viewportRectLeft;
            float right = metrics.viewportRectRight;
            float top = metrics.viewportRectTop;
            float bottom = metrics.viewportRectBottom;

            
            
            if (velocity != null && velocity.length() > 0) {
                if (velocity.x < 0) {
                    left += velocity.x * PREDICTION_VELOCITY_MULTIPLIER;
                } else if (velocity.x > 0) {
                    right += velocity.x * PREDICTION_VELOCITY_MULTIPLIER;
                }

                if (velocity.y < 0) {
                    top += velocity.y * PREDICTION_VELOCITY_MULTIPLIER;
                } else if (velocity.y > 0) {
                    bottom += velocity.y * PREDICTION_VELOCITY_MULTIPLIER;
                }
            }

            
            
            float dangerZoneX = metrics.getWidth() * DANGER_ZONE_MULTIPLIER;
            float dangerZoneY = metrics.getHeight() * DANGER_ZONE_MULTIPLIER;
            left -= dangerZoneX;
            top -= dangerZoneY;
            right += dangerZoneX;
            bottom += dangerZoneY;

            
            
            if (left < 0) left = 0;
            if (top < 0) top = 0;
            if (right > metrics.pageSizeWidth) right = metrics.pageSizeWidth;
            if (bottom > metrics.pageSizeHeight) bottom = metrics.pageSizeHeight;

            RectF predictedViewport = new RectF(left, top, right, bottom);
            return !displayPort.contains(predictedViewport);
        }
    }
}
