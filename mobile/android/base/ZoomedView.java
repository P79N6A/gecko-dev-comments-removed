




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.PanZoomController;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.mozglue.DirectBufferAllocator;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.PointF;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.RelativeLayout;

import java.nio.ByteBuffer;
import java.text.DecimalFormat;

public class ZoomedView extends FrameLayout implements LayerView.OnMetricsChangedListener,
        LayerView.ZoomedViewListener, GeckoEventListener {
    private static final String LOGTAG = "Gecko" + ZoomedView.class.getSimpleName();

    private static final int DEFAULT_ZOOM_FACTOR = 3;
    private static final int W_CAPTURED_VIEW_IN_PERCENT = 80;
    private static final int H_CAPTURED_VIEW_IN_PERCENT = 50;
    private static final int MINIMUM_DELAY_BETWEEN_TWO_RENDER_CALLS_NS = 1000000;
    private static final int DELAY_BEFORE_NEXT_RENDER_REQUEST_MS = 2000;

    private int zoomFactor;
    private ImageView zoomedImageView;
    private LayerView layerView;
    private int viewWidth;
    private int viewHeight;
    private Point lastPosition;
    private boolean shouldSetVisibleOnUpdate;
    private PointF returnValue;

    private boolean stopUpdateView;

    private int lastOrientation;

    private ByteBuffer buffer;
    private Runnable requestRenderRunnable;
    private long startTimeReRender;
    private long lastStartTimeReRender;

    private class ZoomedViewTouchListener implements View.OnTouchListener {
        private float originRawX;
        private float originRawY;
        private boolean dragged;
        private MotionEvent actionDownEvent;

        @Override
        public boolean onTouch(View view, MotionEvent event) {
            if (layerView == null) {
                return false;
            }

            switch (event.getAction()) {
            case MotionEvent.ACTION_MOVE:
                if (moveZoomedView(event)) {
                    dragged = true;
                }
                break;

            case MotionEvent.ACTION_UP:
                if (dragged) {
                    dragged = false;
                } else {
                    layerView.dispatchTouchEvent(actionDownEvent);
                    actionDownEvent.recycle();
                    PointF convertedPosition = getUnzoomedPositionFromPointInZoomedView(event.getX(), event.getY());
                    MotionEvent e = MotionEvent.obtain(event.getDownTime(), event.getEventTime(),
                            MotionEvent.ACTION_UP, convertedPosition.x, convertedPosition.y,
                            event.getMetaState());
                    layerView.dispatchTouchEvent(e);
                    e.recycle();
                }
                break;

            case MotionEvent.ACTION_DOWN:
                dragged = false;
                originRawX = event.getRawX();
                originRawY = event.getRawY();
                PointF convertedPosition = getUnzoomedPositionFromPointInZoomedView(event.getX(), event.getY());
                actionDownEvent = MotionEvent.obtain(event.getDownTime(), event.getEventTime(),
                        MotionEvent.ACTION_DOWN, convertedPosition.x, convertedPosition.y,
                        event.getMetaState());
                break;
            }
            return true;
        }

        private boolean moveZoomedView(MotionEvent event) {
            RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) ZoomedView.this.getLayoutParams();
            if ((!dragged) && (Math.abs((int) (event.getRawX() - originRawX)) < PanZoomController.CLICK_THRESHOLD)
                    && (Math.abs((int) (event.getRawY() - originRawY)) < PanZoomController.CLICK_THRESHOLD)) {
                
                
                return false;
            }

            float newLeftMargin = params.leftMargin + event.getRawX() - originRawX;
            float newTopMargin = params.topMargin + event.getRawY() - originRawY;
            ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
            ZoomedView.this.moveZoomedView(metrics, newLeftMargin, newTopMargin);
            originRawX = event.getRawX();
            originRawY = event.getRawY();
            return true;
        }
    }

    public ZoomedView(Context context) {
        this(context, null, 0);
    }

    public ZoomedView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ZoomedView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        returnValue = new PointF();
        requestRenderRunnable = new Runnable() {
            @Override
            public void run() {
                requestZoomedViewRender();
            }
        };
        EventDispatcher.getInstance().registerGeckoThreadListener(this, "Gesture:nothingDoneOnLongPress",
                "Gesture:clusteredLinksClicked", "Window:Resize", "Content:LocationChange");
    }

    void destroy() {
        ThreadUtils.removeCallbacksFromUiThread(requestRenderRunnable);
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this, "Gesture:nothingDoneOnLongPress",
                "Gesture:clusteredLinksClicked", "Window:Resize", "Content:LocationChange");
    }

    
    
    
    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        ImageView closeButton = (ImageView) findViewById(R.id.dialog_close);
        closeButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                stopZoomDisplay();
            }
        });

        zoomedImageView = (ImageView) findViewById(R.id.zoomed_image_view);
        zoomedImageView.setOnTouchListener(new ZoomedViewTouchListener());
    }

    



    private PointF getUnzoomedPositionFromPointInZoomedView(float x, float y) {
        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        PointF offset = metrics.getMarginOffset();
        final float parentWidth = metrics.getWidth();
        final float parentHeight = metrics.getHeight();
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) getLayoutParams();

        returnValue.x = (int) ((x / zoomFactor) +     

                        offset.x +               

                        





                        (((float) params.leftMargin) - offset.x) *
                            ((parentWidth - offset.x - (viewWidth / zoomFactor)) /
                            (parentWidth - offset.x - viewWidth)));

        
        returnValue.y = (int) ((y / zoomFactor) +
                        offset.y +
                        (((float) params.topMargin) - offset.y) *
                            ((parentHeight - offset.y - (viewHeight / zoomFactor)) /
                            (parentHeight - offset.y - viewHeight)));

        return returnValue;
    }

    




    private PointF getZoomedViewTopLeftPositionFromTouchPosition(float x, float y) {
        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        PointF offset = metrics.getMarginOffset();
        final float parentWidth = metrics.getWidth();
        final float parentHeight = metrics.getHeight();

        returnValue.x = (int) ((((x - (viewWidth / (2 * zoomFactor)))) /   
                                                                        

                        





                        ((parentWidth - offset.x - (viewWidth / zoomFactor)) /
                        (parentWidth - offset.x - viewWidth)))

                + offset.x);     

        
        returnValue.y = (int) ((((y - (viewHeight / (2 * zoomFactor)))) /
                        ((parentHeight - offset.y - (viewHeight / zoomFactor)) /
                        (parentHeight - offset.y - viewHeight)))
                + offset.y);

        return returnValue;
    }

    private void moveZoomedView(ImmutableViewportMetrics metrics, float newLeftMargin, float newTopMargin) {
        final float parentWidth = metrics.getWidth();
        final float parentHeight = metrics.getHeight();
        RelativeLayout.LayoutParams newLayoutParams = (RelativeLayout.LayoutParams) getLayoutParams();
        newLayoutParams.leftMargin = (int) newLeftMargin;
        newLayoutParams.topMargin = (int) newTopMargin;
        int topMarginMin;
        int leftMarginMin;
        PointF offset = metrics.getMarginOffset();
        topMarginMin = (int) offset.y;
        leftMarginMin = (int) offset.x;

        if (newTopMargin < topMarginMin) {
            newLayoutParams.topMargin = topMarginMin;
        } else if (newTopMargin + viewHeight > parentHeight) {
            newLayoutParams.topMargin = (int) (parentHeight - viewHeight);
        }

        if (newLeftMargin < leftMarginMin) {
            newLayoutParams.leftMargin = leftMarginMin;
        } else if (newLeftMargin + viewWidth > parentWidth) {
            newLayoutParams.leftMargin = (int) (parentWidth - viewWidth);
        }

        setLayoutParams(newLayoutParams);
        PointF convertedPosition = getUnzoomedPositionFromPointInZoomedView(0, 0);
        lastPosition = PointUtils.round(convertedPosition);
        requestZoomedViewRender();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        
        
        
        if (lastOrientation != newConfig.orientation) {
            shouldBlockUpdate(true);
            lastOrientation = newConfig.orientation;
        }
    }

    private void refreshZoomedViewSize(ImmutableViewportMetrics viewport) {
        if (layerView == null) {
            return;
        }

        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) getLayoutParams();
        setCapturedSize(viewport);
        moveZoomedView(viewport, params.leftMargin, params.topMargin);
    }

    private void setCapturedSize(ImmutableViewportMetrics metrics) {
        float parentMinSize = Math.min(metrics.getWidth(), metrics.getHeight());
        
        
        
        
        zoomFactor = Math.max(DEFAULT_ZOOM_FACTOR, (int) (DEFAULT_ZOOM_FACTOR / metrics.zoomFactor));
        viewWidth = (int) (parentMinSize * W_CAPTURED_VIEW_IN_PERCENT / (zoomFactor * 100.0)) * zoomFactor;
        viewHeight = (int) (parentMinSize * H_CAPTURED_VIEW_IN_PERCENT / (zoomFactor * 100.0)) * zoomFactor;
        
        
        viewWidth &= ~0x1;
    }

    private void shouldBlockUpdate(boolean shouldBlockUpdate) {
        stopUpdateView = shouldBlockUpdate;
    }

    private Bitmap.Config getBitmapConfig() {
        return (GeckoAppShell.getScreenDepth() == 24) ? Bitmap.Config.ARGB_8888 : Bitmap.Config.RGB_565;
    }

    private void startZoomDisplay(LayerView aLayerView, final int leftFromGecko, final int topFromGecko) {
        if (layerView == null) {
            layerView = aLayerView;
            layerView.addZoomedViewListener(this);
            layerView.setOnMetricsChangedZoomedViewportListener(this);
            ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
            setCapturedSize(metrics);
        }
        startTimeReRender = 0;
        shouldSetVisibleOnUpdate = true;
        moveUsingGeckoPosition(leftFromGecko, topFromGecko);
    }

    private void stopZoomDisplay() {
        shouldSetVisibleOnUpdate = false;
        this.setVisibility(View.GONE);
        ThreadUtils.removeCallbacksFromUiThread(requestRenderRunnable);
        if (layerView != null) {
            layerView.setOnMetricsChangedZoomedViewportListener(null);
            layerView.removeZoomedViewListener(this);
            layerView = null;
        }
    }

    @Override
    public void handleMessage(final String event, final JSONObject message) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (event.equals("Gesture:nothingDoneOnLongPress") || event.equals("Gesture:clusteredLinksClicked")) {
                        final JSONObject clickPosition = message.getJSONObject("clickPosition");
                        int left = clickPosition.getInt("x");
                        int top = clickPosition.getInt("y");
                        
                        LayerView geckoAppLayerView = GeckoAppShell.getLayerView();
                        if (geckoAppLayerView != null) {
                            startZoomDisplay(geckoAppLayerView, left, top);
                        }
                    } else if (event.equals("Window:Resize")) {
                        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
                        refreshZoomedViewSize(metrics);
                    } else if (event.equals("Content:LocationChange")) {
                        stopZoomDisplay();
                    }
                } catch (JSONException e) {
                    Log.e(LOGTAG, "JSON exception", e);
                }
            }
        });
    }

    private void moveUsingGeckoPosition(int leftFromGecko, int topFromGecko) {
        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        PointF convertedPosition = getZoomedViewTopLeftPositionFromTouchPosition((leftFromGecko * metrics.zoomFactor),
                (topFromGecko * metrics.zoomFactor));
        moveZoomedView(metrics, convertedPosition.x, convertedPosition.y);
    }

    @Override
    public void onMetricsChanged(final ImmutableViewportMetrics viewport) {
        
        
        
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                shouldBlockUpdate(false);
                refreshZoomedViewSize(viewport);
            }
        });
    }

    @Override
    public void onPanZoomStopped() {
    }

    @Override
    public void updateView(ByteBuffer data) {
        final Bitmap sb3 = Bitmap.createBitmap(viewWidth, viewHeight, getBitmapConfig());
        if (sb3 != null) {
            data.rewind();
            try {
                sb3.copyPixelsFromBuffer(data);
            } catch (Exception iae) {
                Log.w(LOGTAG, iae.toString());
            }
            BitmapDrawable ob3 = new BitmapDrawable(getResources(), sb3);
            if (zoomedImageView != null) {
                zoomedImageView.setImageDrawable(ob3);
            }
        }
        if (shouldSetVisibleOnUpdate) {
            this.setVisibility(View.VISIBLE);
            shouldSetVisibleOnUpdate = false;
        }
        lastStartTimeReRender = startTimeReRender;
        startTimeReRender = 0;
    }

    private void updateBufferSize() {
        int pixelSize = (GeckoAppShell.getScreenDepth() == 24) ? 4 : 2;
        int capacity = viewWidth * viewHeight * pixelSize;
        if (buffer == null || buffer.capacity() != capacity) {
            buffer = DirectBufferAllocator.free(buffer);
            buffer = DirectBufferAllocator.allocate(capacity);
        }
    }

    private boolean isRendering() {
        return (startTimeReRender != 0);
    }

    private boolean renderFrequencyTooHigh() {
        return ((System.nanoTime() - lastStartTimeReRender) < MINIMUM_DELAY_BETWEEN_TWO_RENDER_CALLS_NS);
    }

    @Override
    public void requestZoomedViewRender() {
        if (stopUpdateView) {
            return;
        }
        
        ThreadUtils.removeCallbacksFromUiThread(requestRenderRunnable);

        
        
        
        
        

        
        if (isRendering()) {
            
            
            
            
            ThreadUtils.postDelayedToUiThread(requestRenderRunnable, DELAY_BEFORE_NEXT_RENDER_REQUEST_MS);
            return;
        }

        
        if (renderFrequencyTooHigh()) {
            
            
            
            
            ThreadUtils.postDelayedToUiThread(requestRenderRunnable, DELAY_BEFORE_NEXT_RENDER_REQUEST_MS);
            return;
        }

        startTimeReRender = System.nanoTime();
        
        
        updateBufferSize();

        int tabId = Tabs.getInstance().getSelectedTab().getId();

        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        PointF origin = metrics.getOrigin();
        PointF offset = metrics.getMarginOffset();

        final int xPos = (int) (origin.x - offset.x) + lastPosition.x;
        final int yPos = (int) (origin.y - offset.y) + lastPosition.y;

        GeckoEvent e = GeckoEvent.createZoomedViewEvent(tabId, xPos, yPos, viewWidth,
                viewHeight, zoomFactor * metrics.zoomFactor, buffer);
        GeckoAppShell.sendEventToGecko(e);
    }

}
