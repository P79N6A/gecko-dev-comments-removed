



package org.mozilla.gecko;

import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.Layer.RenderContext;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.FloatUtils;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONArray;
import org.json.JSONObject;

import android.util.Log;
import android.view.View;

class TextSelection extends Layer implements GeckoEventListener {
    private static final String LOGTAG = "GeckoTextSelection";

    private final TextSelectionHandle mStartHandle;
    private final TextSelectionHandle mMiddleHandle;
    private final TextSelectionHandle mEndHandle;
    private final EventDispatcher mEventDispatcher;

    private float mViewLeft;
    private float mViewTop;
    private float mViewZoom;

    TextSelection(TextSelectionHandle startHandle,
                  TextSelectionHandle middleHandle,
                  TextSelectionHandle endHandle,
                  EventDispatcher eventDispatcher) {
        mStartHandle = startHandle;
        mMiddleHandle = middleHandle;
        mEndHandle = endHandle;
        mEventDispatcher = eventDispatcher;

        
        if (mStartHandle == null || mMiddleHandle == null || mEndHandle == null) {
            Log.e(LOGTAG, "Failed to initialize text selection because at least one handle is null");
        } else {
            registerEventListener("TextSelection:ShowHandles");
            registerEventListener("TextSelection:HideHandles");
            registerEventListener("TextSelection:PositionHandles");
        }
    }

    void destroy() {
        unregisterEventListener("TextSelection:ShowHandles");
        unregisterEventListener("TextSelection:HideHandles");
        unregisterEventListener("TextSelection:PositionHandles");
    }

    private TextSelectionHandle getHandle(String name) {
        if (name.equals("START")) {
            return mStartHandle;
        } else if (name.equals("MIDDLE")) {
            return mMiddleHandle;
        } else {
            return mEndHandle;
        }
    }

    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("TextSelection:ShowHandles")) {
                final JSONArray handles = message.getJSONArray("handles");
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        try {
                            for (int i=0; i < handles.length(); i++) {
                                String handle = handles.getString(i);
                                getHandle(handle).setVisibility(View.VISIBLE);
                            }

                            mViewLeft = 0.0f;
                            mViewTop = 0.0f;
                            mViewZoom = 0.0f;
                            LayerView layerView = GeckoApp.mAppContext.getLayerView();
                            if (layerView != null) {
                                layerView.addLayer(TextSelection.this);
                            }
                        } catch(Exception e) {}
                    }
                });
            } else if (event.equals("TextSelection:HideHandles")) {
                final JSONArray handles = message.getJSONArray("handles");
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        try {
                            LayerView layerView = GeckoApp.mAppContext.getLayerView();
                            if (layerView != null) {
                                layerView.removeLayer(TextSelection.this);
                            }

                            for (int i=0; i < handles.length(); i++) {
                                String handle = handles.getString(i);
                                getHandle(handle).setVisibility(View.GONE);
                            }

                        } catch(Exception e) {}
                    }
                });
            } else if (event.equals("TextSelection:PositionHandles")) {
                final JSONArray positions = message.getJSONArray("positions");
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        try {
                            for (int i=0; i < positions.length(); i++) {
                                JSONObject position = positions.getJSONObject(i);
                                String handle = position.getString("handle");
                                int left = position.getInt("left");
                                int top = position.getInt("top");

                                getHandle(handle).positionFromGecko(left, top);
                             }
                        } catch (Exception e) { }
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    @Override
    public void draw(final RenderContext context) {
        
        
        
        if (FloatUtils.fuzzyEquals(mViewLeft, context.viewport.left)
                && FloatUtils.fuzzyEquals(mViewTop, context.viewport.top)
                && FloatUtils.fuzzyEquals(mViewZoom, context.zoomFactor)) {
            return;
        }
        mViewLeft = context.viewport.left;
        mViewTop = context.viewport.top;
        mViewZoom = context.zoomFactor;

        GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
            public void run() {
                mStartHandle.repositionWithViewport(context.viewport.left, context.viewport.top, context.zoomFactor);
                mMiddleHandle.repositionWithViewport(context.viewport.left, context.viewport.top, context.zoomFactor);
                mEndHandle.repositionWithViewport(context.viewport.left, context.viewport.top, context.zoomFactor);
            }
        });
    }

    private void registerEventListener(String event) {
        mEventDispatcher.registerEventListener(event, this);
    }

    private void unregisterEventListener(String event) {
        mEventDispatcher.unregisterEventListener(event, this);
    }
}
