





































package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.graphics.Rect;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.json.JSONException;
import org.json.JSONObject;
import android.util.Log;





public class ViewportMetrics {
    private IntSize mPageSize;
    private Rect mViewportRect;
    private Point mViewportOffset;

    public ViewportMetrics() {
        mPageSize = new IntSize(LayerController.TILE_WIDTH,
                                LayerController.TILE_HEIGHT);
        mViewportRect = new Rect(0, 0, 1, 1);
        mViewportOffset = new Point(0, 0);
    }

    public ViewportMetrics(ViewportMetrics viewport) {
        mPageSize = new IntSize(viewport.getPageSize());
        mViewportRect = new Rect(viewport.getViewport());
        mViewportOffset = new Point(viewport.getViewportOffset());
    }

    public ViewportMetrics(JSONObject json) throws JSONException {
        int x = json.getInt("x");
        int y = json.getInt("y");
        int width = json.getInt("width");
        int height = json.getInt("height");
        int pageWidth = json.getInt("pageWidth");
        int pageHeight = json.getInt("pageHeight");
        int offsetX = json.getInt("offsetX");
        int offsetY = json.getInt("offsetY");

        mPageSize = new IntSize(pageWidth, pageHeight);
        mViewportRect = new Rect(x, y, x + width, y + height);
        mViewportOffset = new Point(offsetX, offsetY);
    }

    public Point getOptimumViewportOffset() {
        
        
        
        Point optimumOffset =
            new Point((LayerController.TILE_WIDTH - mViewportRect.width()) / 2,
                      (LayerController.TILE_HEIGHT - mViewportRect.height()) / 2);

        



        Rect viewport = getClampedViewport();

        
        
        
        if (viewport.left - optimumOffset.x < 0)
          optimumOffset.x = viewport.left;
        else if (optimumOffset.x + viewport.right > mPageSize.width)
          optimumOffset.x -= (mPageSize.width - (optimumOffset.x + viewport.right));

        if (viewport.top - optimumOffset.y < 0)
          optimumOffset.y = viewport.top;
        else if (optimumOffset.y + viewport.bottom > mPageSize.height)
          optimumOffset.y -= (mPageSize.height - (optimumOffset.y + viewport.bottom));

        return optimumOffset;
    }

    public Point getOrigin() {
        return new Point(mViewportRect.left, mViewportRect.top);
    }

    public Point getDisplayportOrigin() {
        return new Point(mViewportRect.left - mViewportOffset.x,
                         mViewportRect.top - mViewportOffset.y);
    }

    public IntSize getSize() {
        return new IntSize(mViewportRect.width(), mViewportRect.height());
    }

    public Rect getViewport() {
        return mViewportRect;
    }

    
    public Rect getClampedViewport() {
        Rect clampedViewport = new Rect(mViewportRect);

        
        
        
        if (clampedViewport.right > mPageSize.width)
            clampedViewport.offset(mPageSize.width - clampedViewport.right, 0);
        if (clampedViewport.left < 0)
            clampedViewport.offset(-clampedViewport.left, 0);

        if (clampedViewport.bottom > mPageSize.height)
            clampedViewport.offset(0, mPageSize.height - clampedViewport.bottom);
        if (clampedViewport.top < 0)
            clampedViewport.offset(0, -clampedViewport.top);

        return clampedViewport;
    }

    public Point getViewportOffset() {
        return mViewportOffset;
    }

    public IntSize getPageSize() {
        return mPageSize;
    }

    public void setPageSize(IntSize pageSize) {
        mPageSize = pageSize;
    }

    public void setViewport(Rect viewport) {
        mViewportRect = viewport;
    }

    public void setOrigin(Point origin) {
        mViewportRect.set(origin.x, origin.y,
                          origin.x + mViewportRect.width(),
                          origin.y + mViewportRect.height());
    }

    public void setSize(IntSize size) {
        mViewportRect.right = mViewportRect.left + size.width;
        mViewportRect.bottom = mViewportRect.top + size.height;
    }

    public void setViewportOffset(Point offset) {
        mViewportOffset = offset;
    }

    public boolean equals(ViewportMetrics viewport) {
        return mViewportRect.equals(viewport.getViewport()) &&
               mPageSize.equals(viewport.getPageSize()) &&
               mViewportOffset.equals(viewport.getViewportOffset());
    }

    public String toJSON() {
        return "{ \"x\" : " + mViewportRect.left +
               ", \"y\" : " + mViewportRect.top +
               ", \"width\" : " + mViewportRect.width() +
               ", \"height\" : " + mViewportRect.height() +
               ", \"pageWidth\" : " + mPageSize.width +
               ", \"pageHeight\" : " + mPageSize.height +
               ", \"offsetX\" : " + mViewportOffset.x +
               ", \"offsetY\" : " + mViewportOffset.y +
               "}";
    }
}

