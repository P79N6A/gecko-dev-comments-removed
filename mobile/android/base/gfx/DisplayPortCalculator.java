




package org.mozilla.gecko.gfx;

import android.graphics.RectF;
import org.mozilla.gecko.FloatUtils;

final class DisplayPortCalculator {
    private static final String LOGTAG = "GeckoDisplayPortCalculator";

    private static final int DEFAULT_DISPLAY_PORT_MARGIN = 300;

    

    private static final int DANGER_ZONE_X = 75;
    private static final int DANGER_ZONE_Y = 150;

    static DisplayPortMetrics calculate(ImmutableViewportMetrics metrics) {
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

    
    static boolean aboutToCheckerboard(ImmutableViewportMetrics metrics, DisplayPortMetrics displayPort) {
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
