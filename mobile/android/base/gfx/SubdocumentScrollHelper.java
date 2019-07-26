




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONException;
import org.json.JSONObject;

import android.graphics.PointF;
import android.os.Handler;
import android.util.Log;

class SubdocumentScrollHelper implements GeckoEventListener {
    private static final String LOGTAG = "GeckoSubdocumentScrollHelper";

    private static String MESSAGE_PANNING_OVERRIDE = "Panning:Override";
    private static String MESSAGE_CANCEL_OVERRIDE = "Panning:CancelOverride";
    private static String MESSAGE_SCROLL = "Gesture:Scroll";
    private static String MESSAGE_SCROLL_ACK = "Gesture:ScrollAck";

    private final Handler mUiHandler;
    private final EventDispatcher mEventDispatcher;

    

    private final PointF mPendingDisplacement;

    
    private boolean mOverridePanning;

    


    private boolean mOverrideScrollAck;

    

    private boolean mOverrideScrollPending;

    

    private boolean mScrollSucceeded;

    SubdocumentScrollHelper(EventDispatcher eventDispatcher) {
        
        mUiHandler = new Handler();
        mPendingDisplacement = new PointF();

        mEventDispatcher = eventDispatcher;
        registerEventListener(MESSAGE_PANNING_OVERRIDE);
        registerEventListener(MESSAGE_CANCEL_OVERRIDE);
        registerEventListener(MESSAGE_SCROLL_ACK);
    }

    void destroy() {
        unregisterEventListener(MESSAGE_PANNING_OVERRIDE);
        unregisterEventListener(MESSAGE_CANCEL_OVERRIDE);
        unregisterEventListener(MESSAGE_SCROLL_ACK);
    }

    private void registerEventListener(String event) {
        mEventDispatcher.registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        mEventDispatcher.unregisterEventListener(event, this);
    }

    boolean scrollBy(PointF displacement) {
        if (! mOverridePanning) {
            return false;
        }

        if (! mOverrideScrollAck) {
            mOverrideScrollPending = true;
            mPendingDisplacement.x += displacement.x;
            mPendingDisplacement.y += displacement.y;
            return true;
        }

        JSONObject json = new JSONObject();
        try {
            json.put("x", displacement.x);
            json.put("y", displacement.y);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error forming subwindow scroll message: ", e);
        }
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(MESSAGE_SCROLL, json.toString()));

        mOverrideScrollAck = false;
        mOverrideScrollPending = false;
        
        
        mPendingDisplacement.x = 0;
        mPendingDisplacement.y = 0;

        return true;
    }

    void cancel() {
        mOverridePanning = false;
    }

    boolean scrolling() {
        return mOverridePanning;
    }

    boolean lastScrollSucceeded() {
        return mScrollSucceeded;
    }

    

    @Override
    public void handleMessage(final String event, final JSONObject message) {
        
        mUiHandler.post(new Runnable() {
            @Override
            public void run() {
                Log.i(LOGTAG, "Got message: " + event);
                try {
                    if (MESSAGE_PANNING_OVERRIDE.equals(event)) {
                        mOverridePanning = true;
                        mOverrideScrollAck = true;
                        mOverrideScrollPending = false;
                        mScrollSucceeded = true;
                    } else if (MESSAGE_CANCEL_OVERRIDE.equals(event)) {
                        mOverridePanning = false;
                    } else if (MESSAGE_SCROLL_ACK.equals(event)) {
                        mOverrideScrollAck = true;
                        mScrollSucceeded = message.getBoolean("scrolled");
                        if (mOverridePanning && mOverrideScrollPending) {
                            scrollBy(mPendingDisplacement);
                        }
                    }
                } catch (Exception e) {
                    Log.e(LOGTAG, "Exception handling message", e);
                }
            }
        });
    }
}
