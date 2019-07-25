




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.InputConnectionHandler;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.ui.SimpleScaleGestureDetector;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.util.Log;
import java.util.LinkedList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;







public class LayerView extends GLSurfaceView
    implements GeckoEventListener {
    private Context mContext;
    private LayerController mController;
    private InputConnectionHandler mInputConnectionHandler;
    private LayerRenderer mRenderer;
    private GestureDetector mGestureDetector;
    private SimpleScaleGestureDetector mScaleGestureDetector;
    private long mRenderTime;
    private boolean mRenderTimeReset;
    private static String LOGTAG = "GeckoLayerView";
    
    private LinkedList<MotionEvent> mEventQueue = new LinkedList<MotionEvent>();
    private boolean sendTouchEvents = false;
    private String touchEventsPrefName = "dom.w3c_touch_events.enabled";

    public LayerView(Context context, LayerController controller) {
        super(context);

        mContext = context;
        mController = controller;
        mRenderer = new LayerRenderer(this);
        setRenderer(mRenderer);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
        mGestureDetector = new GestureDetector(context, controller.getGestureListener());
        mScaleGestureDetector =
            new SimpleScaleGestureDetector(controller.getScaleGestureListener());
        mGestureDetector.setOnDoubleTapListener(controller.getDoubleTapListener());
        mInputConnectionHandler = null;

        setFocusable(true);
        setFocusableInTouchMode(true);

        GeckoAppShell.registerGeckoEventListener("Preferences:Data", this);
        JSONArray jsonPrefs = new JSONArray();
        jsonPrefs.put(touchEventsPrefName);
        GeckoEvent event = new GeckoEvent("Preferences:Get", jsonPrefs.toString());
        GeckoAppShell.sendEventToGecko(event);
    }

    public void handleMessage(String event, JSONObject message) {
        if (event.equals("Preferences:Data")) {
            try {
                JSONArray jsonPrefs = message.getJSONArray("preferences");
                for (int i = 0; i < jsonPrefs.length(); i++) {
                    JSONObject jPref = jsonPrefs.getJSONObject(i);
                    final String prefName = jPref.getString("name");
                    if (prefName.equals(touchEventsPrefName)) {
                        sendTouchEvents = jPref.getBoolean("value");
                        GeckoAppShell.unregisterGeckoEventListener("Preferences:Data", this);
                    }
                }
            } catch(JSONException ex) {
                Log.e(LOGTAG, "Error decoding JSON", ex);
            }
        }
    }

    private void addToEventQueue(MotionEvent event) {
        MotionEvent copy = MotionEvent.obtain(event);
        mEventQueue.add(copy);
    }

    public void processEventQueue() {
        MotionEvent event = mEventQueue.poll();
        while(event != null) {
            processEvent(event);
            event = mEventQueue.poll();
        }
    }

    public void clearEventQueue() {
        mEventQueue.clear();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (sendTouchEvents && mController.onTouchEvent(event)) {
            addToEventQueue(event);
            return true;
        }
        return processEvent(event);
    }

    private boolean processEvent(MotionEvent event) {
        if (mGestureDetector.onTouchEvent(event))
            return true;
        mScaleGestureDetector.onTouchEvent(event);
        if (mScaleGestureDetector.isInProgress())
            return true;
        mController.getPanZoomController().onTouchEvent(event);
        return true;
    }

    public LayerController getController() { return mController; }

    
    public void setViewportSize(IntSize size) {
        mController.setViewportSize(new FloatSize(size));
    }

    public void setInputConnectionHandler(InputConnectionHandler handler) {
        mInputConnectionHandler = handler;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onCreateInputConnection(outAttrs);
        return null;
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyPreIme(keyCode, event);
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyDown(keyCode, event);
        return false;
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyLongPress(keyCode, event);
        return false;
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyMultiple(keyCode, repeatCount, event);
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyUp(keyCode, event);
        return false;
    }

    @Override
    public void requestRender() {
        super.requestRender();

        synchronized(this) {
            if (!mRenderTimeReset) {
                mRenderTimeReset = true;
                mRenderTime = System.nanoTime();
            }
        }
    }

    



    public long getRenderTime() {
        synchronized(this) {
            mRenderTimeReset = false;
            return System.nanoTime() - mRenderTime;
        }
    }

    public int getMaxTextureSize() {
        return mRenderer.getMaxTextureSize();
    }
}

