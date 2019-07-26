 



package org.mozilla.gecko;

import org.mozilla.gecko.gfx.ImmutableViewportMetrics;
import org.mozilla.gecko.gfx.LayerView;

import org.json.JSONObject;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.PointF;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.RelativeLayout;

class TextSelectionHandle extends ImageView implements View.OnTouchListener {
    private static final String LOGTAG = "GeckoTextSelectionHandle";

    private enum HandleType { START, MIDDLE, END }; 

    private final HandleType mHandleType;
    private final int mWidth;
    private final int mHeight;
    private final int mShadow;

    private float mLeft;
    private float mTop;
    private boolean mIsRTL; 
    private PointF mGeckoPoint;
    private float mTouchStartX;
    private float mTouchStartY;

    private RelativeLayout.LayoutParams mLayoutParams;

    private static final int IMAGE_LEVEL_LTR = 0;
    private static final int IMAGE_LEVEL_RTL = 1;

    private GeckoApp mActivity;

    TextSelectionHandle(Context context, AttributeSet attrs) {
        super(context, attrs);
        setOnTouchListener(this);
        mActivity = (GeckoApp) context;

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TextSelectionHandle);
        int handleType = a.getInt(R.styleable.TextSelectionHandle_handleType, 0x01);

        if (handleType == 0x01)
            mHandleType = HandleType.START;
        else if (handleType == 0x02)
            mHandleType = HandleType.MIDDLE;
        else
            mHandleType = HandleType.END;

        mIsRTL = false;
        mGeckoPoint = new PointF(0.0f, 0.0f);

        mWidth = getResources().getDimensionPixelSize(R.dimen.text_selection_handle_width);
        mHeight = getResources().getDimensionPixelSize(R.dimen.text_selection_handle_height);
        mShadow = getResources().getDimensionPixelSize(R.dimen.text_selection_handle_shadow);
    }

    public boolean onTouch(View v, MotionEvent event) {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                mTouchStartX = event.getX();
                mTouchStartY = event.getY();
                break;
            }
            case MotionEvent.ACTION_UP: {
                mTouchStartX = 0;
                mTouchStartY = 0;

                
                JSONObject args = new JSONObject();
                try {
                    args.put("handleType", mHandleType.toString());
                } catch (Exception e) {
                    Log.e(LOGTAG, "Error building JSON arguments for TextSelection:Position");
                }
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("TextSelection:Position", args.toString()));
                break;
            }
            case MotionEvent.ACTION_MOVE: {
                move(event.getX(), event.getY());
                break;
            }
        }
        return true;
    }

    private void move(float newX, float newY) {
        mLeft = mLeft + newX - mTouchStartX;
        mTop = mTop + newY - mTouchStartY;

        LayerView layerView = mActivity.getLayerView();
        if (layerView == null) {
            Log.e(LOGTAG, "Can't move selection because layerView is null");
            return;
        }
        
        float left = mLeft + adjustLeftForHandle();

        PointF geckoPoint = new PointF(left, mTop);
        geckoPoint = layerView.convertViewPointToLayerPoint(geckoPoint);

        JSONObject args = new JSONObject();
        try {
            args.put("handleType", mHandleType.toString());
            args.put("x", (int) geckoPoint.x);
            args.put("y", (int) geckoPoint.y);
        } catch (Exception e) {
            Log.e(LOGTAG, "Error building JSON arguments for TextSelection:Move");
        }
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("TextSelection:Move", args.toString()));

        setLayoutPosition();
    }

    void positionFromGecko(int left, int top, boolean rtl) {
        LayerView layerView = mActivity.getLayerView();
        if (layerView == null) {
            Log.e(LOGTAG, "Can't position handle because layerView is null");
            return;
        }

        mGeckoPoint = new PointF(left, top);
        if (mIsRTL != rtl) {
            mIsRTL = rtl;
            setImageLevel(mIsRTL ? IMAGE_LEVEL_RTL : IMAGE_LEVEL_LTR);
        }

        ImmutableViewportMetrics metrics = layerView.getViewportMetrics();
        repositionWithViewport(metrics.viewportRectLeft, metrics.viewportRectTop, metrics.zoomFactor);
    }

    void repositionWithViewport(float x, float y, float zoom) {
        PointF viewPoint = new PointF((mGeckoPoint.x * zoom) - x,
                                      (mGeckoPoint.y * zoom) - y);

        mLeft = viewPoint.x - adjustLeftForHandle();
        mTop = viewPoint.y;

        setLayoutPosition();
    }

    private float adjustLeftForHandle() {
        if (mHandleType.equals(HandleType.START))
            return mIsRTL ? mShadow : mWidth - mShadow;
        else if (mHandleType.equals(HandleType.MIDDLE))
            return (mWidth - mShadow) / 2;
        else
            return mIsRTL ? mWidth - mShadow : mShadow;
    }

    private void setLayoutPosition() {
        if (mLayoutParams == null) {
            mLayoutParams = (RelativeLayout.LayoutParams) getLayoutParams();
            
            
            
            mLayoutParams.rightMargin = 0 - mWidth;
            mLayoutParams.bottomMargin = 0 - mHeight;
        }

        mLayoutParams.leftMargin = (int) mLeft;
        mLayoutParams.topMargin = (int) mTop;
        setLayoutParams(mLayoutParams);
    }
}
