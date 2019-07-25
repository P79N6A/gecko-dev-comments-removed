




































package org.mozilla.gecko;

import android.view.GestureDetector;
import android.view.MotionEvent;
import android.content.Context;
import android.view.View;
import org.json.JSONArray;
import org.json.JSONObject;
import android.util.Log;

class GeckoGestureDetector implements GestureDetector.OnGestureListener {
    private GestureDetector mDetector;
    private static final String LOG_FILE_NAME = "GeckoGestureDetector";
    public GeckoGestureDetector(Context aContext) {
        mDetector = new GestureDetector(aContext, this);
    }

    public boolean onTouchEvent(MotionEvent event) {
        return mDetector.onTouchEvent(event);
    }

    @Override
    public boolean onDown(MotionEvent e) {
    	return true;
    }

    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
    	return true;
    }

    @Override
    public void onLongPress(MotionEvent motionEvent) {
        JSONObject ret = new JSONObject();
        try {
            ret.put("x", motionEvent.getX());
            ret.put("y", motionEvent.getY());
        } catch(Exception ex) {
            Log.w(LOG_FILE_NAME, "Error building return: " + ex);
        }

        GeckoEvent e = new GeckoEvent("Gesture:LongPress", ret.toString());
        GeckoAppShell.sendEventToGecko(e);
    }

    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
    	return true;
    }

    @Override
    public void onShowPress(MotionEvent e) {
    }
    
    @Override
    public boolean onSingleTapUp(MotionEvent e) {
    	return true;
    }
}
