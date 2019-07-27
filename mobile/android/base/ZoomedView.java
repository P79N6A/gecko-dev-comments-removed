




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
import android.content.res.Resources;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.Shader;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.OvershootInterpolator;
import android.view.animation.ScaleAnimation;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.nio.ByteBuffer;
import java.text.DecimalFormat;

public class ZoomedView extends FrameLayout implements LayerView.OnMetricsChangedListener,
        LayerView.ZoomedViewListener, GeckoEventListener {
    private static final String LOGTAG = "Gecko" + ZoomedView.class.getSimpleName();

    private static final float[] ZOOM_FACTORS_LIST = {2.0f, 3.0f, 1.5f};
    private static final int W_CAPTURED_VIEW_IN_PERCENT = 50;
    private static final int H_CAPTURED_VIEW_IN_PERCENT = 50;
    private static final int MINIMUM_DELAY_BETWEEN_TWO_RENDER_CALLS_NS = 1000000;
    private static final int DELAY_BEFORE_NEXT_RENDER_REQUEST_MS = 2000;
    private static final int OPENING_ANIMATION_DURATION_MS = 250;
    private static final int CLOSING_ANIMATION_DURATION_MS = 150;
    private static final float OVERSHOOT_INTERPOLATOR_TENSION = 1.5f;

    private float zoomFactor;
    private int currentZoomFactorIndex;
    private ImageView zoomedImageView;
    private LayerView layerView;
    private int viewWidth;
    private int viewHeight; 
    private int viewContainerWidth;
    private int viewContainerHeight; 
    private int containterSize; 
    private Point lastPosition;
    private boolean shouldSetVisibleOnUpdate;
    private PointF returnValue;
    private final PointF animationStart;
    private ImageView closeButton;
    private TextView changeZoomFactorButton;
    private boolean toolbarOnTop;
    private float offsetDueToToolBarPosition;
    private int toolbarHeight;
    private int cornerRadius;

    private boolean stopUpdateView;

    private int lastOrientation;

    private ByteBuffer buffer;
    private Runnable requestRenderRunnable;
    private long startTimeReRender;
    private long lastStartTimeReRender;

    private ZoomedViewTouchListener touchListener;

    private enum StartPointUpdate {
        GECKO_POSITION, CENTER, NO_CHANGE
    }

    private class RoundedBitmapDrawable extends BitmapDrawable {
        private Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG | Paint.DITHER_FLAG);
        final float cornerRadius;
        final boolean squareOnTopOfDrawable;

        RoundedBitmapDrawable(Resources res, Bitmap bitmap, boolean squareOnTop, int radius) {
            super(res, bitmap);
            squareOnTopOfDrawable = squareOnTop;
            final BitmapShader shader = new BitmapShader(bitmap, Shader.TileMode.CLAMP,
                Shader.TileMode.CLAMP);
            paint.setAntiAlias(true);
            paint.setShader(shader);
            cornerRadius = radius;
        }

        @Override
        public void draw(Canvas canvas) {
            int height = getBounds().height();
            int width = getBounds().width();
            RectF rect = new RectF(0.0f, 0.0f, width, height);
            canvas.drawRoundRect(rect, cornerRadius, cornerRadius, paint);

            
            if (squareOnTopOfDrawable) {
                canvas.drawRect(0, 0, cornerRadius, cornerRadius, paint);
                canvas.drawRect(width - cornerRadius, 0, width, cornerRadius, paint);
            } else {
                canvas.drawRect(0, height - cornerRadius, cornerRadius, height, paint);
                canvas.drawRect(width - cornerRadius, height - cornerRadius, width, height, paint);
            }
        }
    }

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
                    if (isClickInZoomedView(event.getY())) {
                        GeckoEvent eClickInZoomedView = GeckoEvent.createBroadcastEvent("Gesture:ClickInZoomedView", "");
                        GeckoAppShell.sendEventToGecko(eClickInZoomedView);
                        layerView.dispatchTouchEvent(actionDownEvent);
                        actionDownEvent.recycle();
                        PointF convertedPosition = getUnzoomedPositionFromPointInZoomedView(event.getX(), event.getY());
                        MotionEvent e = MotionEvent.obtain(event.getDownTime(), event.getEventTime(),
                                MotionEvent.ACTION_UP, convertedPosition.x, convertedPosition.y,
                                event.getMetaState());
                        layerView.dispatchTouchEvent(e);
                        e.recycle();
                    }
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

        private boolean isClickInZoomedView(float y) {
            return ((toolbarOnTop && y > toolbarHeight) ||
                (!toolbarOnTop && y < ZoomedView.this.viewHeight));
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
            ZoomedView.this.moveZoomedView(metrics, newLeftMargin, newTopMargin, StartPointUpdate.CENTER);
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
        animationStart = new PointF();
        currentZoomFactorIndex = 0;
        zoomFactor = ZOOM_FACTORS_LIST[currentZoomFactorIndex];
        requestRenderRunnable = new Runnable() {
            @Override
            public void run() {
                requestZoomedViewRender();
            }
        };
        touchListener = new ZoomedViewTouchListener();
        EventDispatcher.getInstance().registerGeckoThreadListener(this,
                "Gesture:clusteredLinksClicked", "Window:Resize", "Content:LocationChange");
    }

    void destroy() {
        ThreadUtils.removeCallbacksFromUiThread(requestRenderRunnable);
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this,
                "Gesture:clusteredLinksClicked", "Window:Resize", "Content:LocationChange");
    }

    
    
    
    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        closeButton = (ImageView) findViewById(R.id.dialog_close);
        changeZoomFactorButton = (TextView) findViewById(R.id.change_zoom_factor);
        zoomedImageView = (ImageView) findViewById(R.id.zoomed_image_view);

        setTextInZoomFactorButton(ZOOM_FACTORS_LIST[0]);

        toolbarHeight = getResources().getDimensionPixelSize(R.dimen.zoomed_view_toolbar_height);
        containterSize = getResources().getDimensionPixelSize(R.dimen.drawable_dropshadow_size);
        cornerRadius = getResources().getDimensionPixelSize(R.dimen.button_corner_radius);

        moveToolbar(true);
    }

    private void setListeners() {
        closeButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                stopZoomDisplay(true);
            }
        });

        changeZoomFactorButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                changeZoomFactor();
            }
        });

        setOnTouchListener(touchListener);
    }

    private void removeListeners() {
        closeButton.setOnClickListener(null);

        changeZoomFactorButton.setOnClickListener(null);

        setOnTouchListener(null);
    }
    



    private PointF getUnzoomedPositionFromPointInZoomedView(float x, float y) {
        if (toolbarOnTop && y > toolbarHeight) {
           y = y - toolbarHeight;
        }

        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        PointF offset = metrics.getMarginOffset();
        final float parentWidth = metrics.getWidth();
        final float parentHeight = metrics.getHeight();
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) getLayoutParams();

        returnValue.x = (int) ((x / zoomFactor) +     

                        offset.x +               

                        





                        (((float) params.leftMargin) - offset.x) *
                            ((parentWidth - offset.x - (viewWidth / zoomFactor)) /
                            (parentWidth - offset.x - viewContainerWidth)));

        
        returnValue.y = (int) ((y / zoomFactor) +
                        offset.y -
                        offsetDueToToolBarPosition +
                        (((float) params.topMargin) - offset.y) *
                            ((parentHeight - offset.y + offsetDueToToolBarPosition - (viewHeight / zoomFactor)) /
                            (parentHeight - offset.y - viewContainerHeight)));

        return returnValue;
    }

    




    private PointF getZoomedViewTopLeftPositionFromTouchPosition(float x, float y) {
        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        PointF offset = metrics.getMarginOffset();
        final float parentWidth = metrics.getWidth();
        final float parentHeight = metrics.getHeight();

        returnValue.x = (int) ((((x - (viewWidth / (2 * zoomFactor)))) /   
                                                                        

                        





                        ((parentWidth - offset.x - (viewWidth / zoomFactor)) /
                        (parentWidth - offset.x - viewContainerWidth)))

                + offset.x);     

        
        returnValue.y = (int) ((((y + offsetDueToToolBarPosition - (viewHeight / (2 * zoomFactor)))) /
                        ((parentHeight - offset.y + offsetDueToToolBarPosition - (viewHeight / zoomFactor)) /
                        (parentHeight - offset.y - viewContainerHeight)))
                + offset.y);

        return returnValue;
    }

    private void moveZoomedView(ImmutableViewportMetrics metrics, float newLeftMargin, float newTopMargin,
            StartPointUpdate animateStartPoint) {
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
        } else if (newTopMargin + viewContainerHeight > parentHeight) {
            newLayoutParams.topMargin = (int) (parentHeight - viewContainerHeight);
        }

        if (newLeftMargin < leftMarginMin) {
            newLayoutParams.leftMargin = leftMarginMin;
        } else if (newLeftMargin + viewContainerWidth > parentWidth) {
            newLayoutParams.leftMargin = (int) (parentWidth - viewContainerWidth);
        }

        if (newLayoutParams.topMargin < topMarginMin + 1) {
            moveToolbar(false);
        } else if (newLayoutParams.topMargin + viewContainerHeight > parentHeight - 1) {
            moveToolbar(true);
        }

        if (animateStartPoint == StartPointUpdate.GECKO_POSITION) {
            
            
            
            
            animationStart.x = animationStart.x - newLayoutParams.leftMargin;
            animationStart.y = animationStart.y - newLayoutParams.topMargin;
        } else if (animateStartPoint == StartPointUpdate.CENTER) {
            
            
            
            PointF convertedPosition = getUnzoomedPositionFromPointInZoomedView(viewContainerWidth / 2, viewContainerHeight / 2);
            animationStart.x = convertedPosition.x - newLayoutParams.leftMargin;
            animationStart.y = convertedPosition.y - newLayoutParams.topMargin;
        }

        setLayoutParams(newLayoutParams);
        PointF convertedPosition = getUnzoomedPositionFromPointInZoomedView(0, 0);
        lastPosition = PointUtils.round(convertedPosition);
        requestZoomedViewRender();
    }

    private void moveToolbar(boolean moveTop) {
        if (toolbarOnTop == moveTop) {
            return;
        }
        toolbarOnTop = moveTop;
        if (toolbarOnTop) {
            offsetDueToToolBarPosition = toolbarHeight;
        } else {
            offsetDueToToolBarPosition = 0;
        }

        RelativeLayout.LayoutParams p = (RelativeLayout.LayoutParams) zoomedImageView.getLayoutParams();
        RelativeLayout.LayoutParams pChangeZoomFactorButton = (RelativeLayout.LayoutParams) changeZoomFactorButton.getLayoutParams();
        RelativeLayout.LayoutParams pCloseButton = (RelativeLayout.LayoutParams) closeButton.getLayoutParams();

        if (moveTop) {
            p.addRule(RelativeLayout.BELOW, R.id.change_zoom_factor);
            pChangeZoomFactorButton.addRule(RelativeLayout.BELOW, 0);
            pCloseButton.addRule(RelativeLayout.BELOW, 0);
        } else {
            p.addRule(RelativeLayout.BELOW, 0);
            pChangeZoomFactorButton.addRule(RelativeLayout.BELOW, R.id.zoomed_image_view);
            pCloseButton.addRule(RelativeLayout.BELOW, R.id.zoomed_image_view);
        }
        pChangeZoomFactorButton.addRule(RelativeLayout.ALIGN_LEFT, R.id.zoomed_image_view);
        pCloseButton.addRule(RelativeLayout.ALIGN_RIGHT, R.id.zoomed_image_view);
        zoomedImageView.setLayoutParams(p);
        changeZoomFactorButton.setLayoutParams(pChangeZoomFactorButton);
        closeButton.setLayoutParams(pCloseButton);
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
        moveZoomedView(viewport, params.leftMargin, params.topMargin, StartPointUpdate.NO_CHANGE);
    }

    private void setCapturedSize(ImmutableViewportMetrics metrics) {
        float parentMinSize = Math.min(metrics.getWidth(), metrics.getHeight());
        viewWidth = (int) ((parentMinSize * W_CAPTURED_VIEW_IN_PERCENT / (zoomFactor * 100.0)) * zoomFactor);
        viewHeight = (int) ((parentMinSize * H_CAPTURED_VIEW_IN_PERCENT / (zoomFactor * 100.0)) * zoomFactor);
        viewContainerHeight = viewHeight + toolbarHeight +
                2 * containterSize; 
        viewContainerWidth = viewWidth +
                2 * containterSize; 
        
        
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

        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        PointF offset = metrics.getMarginOffset();
        
        
        
        animationStart.x = (float) leftFromGecko * metrics.zoomFactor + offset.x;
        animationStart.y = (float) topFromGecko * metrics.zoomFactor + offset.y;

        moveUsingGeckoPosition(leftFromGecko, topFromGecko);
    }

    private void stopZoomDisplay(boolean withAnimation) {
        shouldSetVisibleOnUpdate = false;
        hideZoomedView(withAnimation);
        ThreadUtils.removeCallbacksFromUiThread(requestRenderRunnable);
        if (layerView != null) {
            layerView.setOnMetricsChangedZoomedViewportListener(null);
            layerView.removeZoomedViewListener(this);
            layerView = null;
        }
    }

    private void changeZoomFactor() {
        if (currentZoomFactorIndex < ZOOM_FACTORS_LIST.length - 1) {
            currentZoomFactorIndex++;
        } else {
            currentZoomFactorIndex = 0;
        }
        zoomFactor = ZOOM_FACTORS_LIST[currentZoomFactorIndex];

        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        refreshZoomedViewSize(metrics);
        setTextInZoomFactorButton(zoomFactor);
    }

    private void setTextInZoomFactorButton(float zoom) {
        final String percentageValue = Integer.toString((int) (100*zoom));
        changeZoomFactorButton.setText(getResources().getString(R.string.percent, percentageValue));
    }

    @Override
    public void handleMessage(final String event, final JSONObject message) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (event.equals("Gesture:clusteredLinksClicked")) {
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
                        stopZoomDisplay(false);
                    }
                } catch (JSONException e) {
                    Log.e(LOGTAG, "JSON exception", e);
                }
            }
        });
    }

    private void moveUsingGeckoPosition(int leftFromGecko, int topFromGecko) {
        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        final float parentHeight = metrics.getHeight();
        
        
        moveToolbar((topFromGecko * metrics.zoomFactor > parentHeight / 2));
        PointF convertedPosition = getZoomedViewTopLeftPositionFromTouchPosition((leftFromGecko * metrics.zoomFactor),
                (topFromGecko * metrics.zoomFactor));
        moveZoomedView(metrics, convertedPosition.x, convertedPosition.y, StartPointUpdate.GECKO_POSITION);
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
            if (zoomedImageView != null) {
                RoundedBitmapDrawable ob3 = new RoundedBitmapDrawable(getResources(), sb3, toolbarOnTop, cornerRadius);
                zoomedImageView.setImageDrawable(ob3);
            }
        }
        if (shouldSetVisibleOnUpdate) {
            this.showZoomedView();
        }
        lastStartTimeReRender = startTimeReRender;
        startTimeReRender = 0;
    }

    private void showZoomedView() {
        
        if (getVisibility() != View.VISIBLE) {
            final Animation anim = new ScaleAnimation(
                    0f, 1f, 
                    0f, 1f, 
                    Animation.ABSOLUTE, animationStart.x, 
                    Animation.ABSOLUTE, animationStart.y); 
            anim.setFillAfter(true); 
            anim.setDuration(OPENING_ANIMATION_DURATION_MS);
            anim.setInterpolator(new OvershootInterpolator(OVERSHOOT_INTERPOLATOR_TENSION));
            anim.setAnimationListener(new AnimationListener() {
                public void onAnimationEnd(Animation animation) {
                    setListeners();
                }
                public void onAnimationRepeat(Animation animation) {
                }
                public void onAnimationStart(Animation animation) {
                    removeListeners();
                }
            });
            setAnimation(anim);
        }
        setVisibility(View.VISIBLE);
        shouldSetVisibleOnUpdate = false;
    }

    private void hideZoomedView(boolean withAnimation) {
        if (withAnimation) {
            final Animation anim = new ScaleAnimation(
                1f, 0f, 
                1f, 0f, 
                Animation.ABSOLUTE, animationStart.x, 
                Animation.ABSOLUTE, animationStart.y); 
            anim.setFillAfter(true); 
            anim.setDuration(CLOSING_ANIMATION_DURATION_MS);
            anim.setAnimationListener(new AnimationListener() {
                public void onAnimationEnd(Animation animation) {
                }
                public void onAnimationRepeat(Animation animation) {
                }
                public void onAnimationStart(Animation animation) {
                    removeListeners();
                }
            });
            setAnimation(anim);
        } else {
            removeListeners();
            setAnimation(null);
        }
        setVisibility(View.GONE);
        shouldSetVisibleOnUpdate = false;
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
