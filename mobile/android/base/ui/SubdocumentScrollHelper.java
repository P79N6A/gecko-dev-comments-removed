




































package org.mozilla.gecko.ui;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import org.json.JSONObject;
import org.json.JSONException;
import android.graphics.PointF;
import android.os.Handler;
import android.util.Log;

class SubdocumentScrollHelper implements GeckoEventListener {
    private static final String LOGTAG = "GeckoSubdocumentScrollHelper";

    private static String MESSAGE_PANNING_OVERRIDE = "Panning:Override";
    private static String MESSAGE_CANCEL_OVERRIDE = "Panning:CancelOverride";
    private static String MESSAGE_SCROLL = "Gesture:Scroll";
    private static String MESSAGE_SCROLL_ACK = "Gesture:ScrollAck";

    private final PanZoomController mPanZoomController;
    private final Handler mUiHandler;

    private boolean mOverridePanning;
    private boolean mOverrideScrollAck;
    private boolean mOverrideScrollPending;

    SubdocumentScrollHelper(PanZoomController controller) {
        mPanZoomController = controller;
        
        mUiHandler = new Handler();

        GeckoAppShell.registerGeckoEventListener(MESSAGE_PANNING_OVERRIDE, this);
        GeckoAppShell.registerGeckoEventListener(MESSAGE_CANCEL_OVERRIDE, this);
        GeckoAppShell.registerGeckoEventListener(MESSAGE_SCROLL_ACK, this);
    }

    boolean scrollBy(PointF displacement) {
        if (! mOverridePanning) {
            return false;
        }

        if (! mOverrideScrollAck) {
            mOverrideScrollPending = true;
            return true;
        }

        mOverrideScrollAck = false;
        mOverrideScrollPending = false;

        JSONObject json = new JSONObject();
        try {
            json.put("x", displacement.x);
            json.put("y", displacement.y);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error forming subwindow scroll message: ", e);
        }
        GeckoAppShell.sendEventToGecko(new GeckoEvent(MESSAGE_SCROLL, json.toString()));

        return true;
    }

    void cancel() {
        mOverridePanning = false;
    }

    boolean scrolling() {
        return mOverridePanning;
    }

    

    public void handleMessage(final String event, final JSONObject message) {
        
        mUiHandler.post(new Runnable() {
            public void run() {
                Log.i(LOGTAG, "Got message: " + event);
                try {
                    if (MESSAGE_PANNING_OVERRIDE.equals(event)) {
                        mOverridePanning = true;
                        mOverrideScrollAck = true;
                        mOverrideScrollPending = false;
                    } else if (MESSAGE_CANCEL_OVERRIDE.equals(event)) {
                        mOverridePanning = false;
                    } else if (MESSAGE_SCROLL_ACK.equals(event)) {
                        mOverrideScrollAck = true;
                        if (mOverridePanning && mOverrideScrollPending) {
                            scrollBy(mPanZoomController.getDisplacement());
                        }
                    }
                } catch (Exception e) {
                    Log.e(LOGTAG, "Exception handling message", e);
                }
            }
        });
    }
}
