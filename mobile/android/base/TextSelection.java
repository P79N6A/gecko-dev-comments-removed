



package org.mozilla.gecko;

import android.util.Log;
import android.view.View;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.Layer.RenderContext;
import org.mozilla.gecko.gfx.LayerController;
import org.json.JSONObject;

class TextSelection extends Layer implements GeckoEventListener {
    private static final String LOGTAG = "GeckoTextSelection";

    private final TextSelectionHandle mStartHandle;
    private final TextSelectionHandle mEndHandle;

    private float mViewLeft;
    private float mViewTop;
    private float mViewZoom;

    TextSelection(TextSelectionHandle startHandle, TextSelectionHandle endHandle) {
        mStartHandle = startHandle;
        mEndHandle = endHandle;

        
        if (mStartHandle == null || mEndHandle == null) {
            Log.e(LOGTAG, "Failed to initialize text selection because at least one handle is null");
        } else {
            GeckoAppShell.registerGeckoEventListener("TextSelection:ShowHandles", this);
            GeckoAppShell.registerGeckoEventListener("TextSelection:HideHandles", this);
            GeckoAppShell.registerGeckoEventListener("TextSelection:PositionHandles", this);
        }
    }

    void destroy() {
        GeckoAppShell.unregisterGeckoEventListener("TextSelection:ShowHandles", this);
        GeckoAppShell.unregisterGeckoEventListener("TextSelection:HideHandles", this);
        GeckoAppShell.unregisterGeckoEventListener("TextSelection:PositionHandles", this);
    }

    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("TextSelection:ShowHandles")) {
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        mStartHandle.setVisibility(View.VISIBLE);
                        mEndHandle.setVisibility(View.VISIBLE);

                        mViewLeft = 0.0f;
                        mViewTop = 0.0f;
                        mViewZoom = 0.0f;
                        LayerController layerController = GeckoApp.mAppContext.getLayerController();
                        if (layerController != null) {
                            layerController.getView().addLayer(TextSelection.this);
                        }
                    }
                });
            } else if (event.equals("TextSelection:HideHandles")) {
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        LayerController layerController = GeckoApp.mAppContext.getLayerController();
                        if (layerController != null) {
                            layerController.getView().removeLayer(TextSelection.this);
                        }

                        mStartHandle.setVisibility(View.GONE);
                        mEndHandle.setVisibility(View.GONE);
                    }
                });
            } else if (event.equals("TextSelection:PositionHandles")) {
                final int startLeft = message.getInt("startLeft");
                final int startTop = message.getInt("startTop");
                final int endLeft = message.getInt("endLeft");
                final int endTop = message.getInt("endTop");

                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        mStartHandle.positionFromGecko(startLeft, startTop);
                        mEndHandle.positionFromGecko(endLeft, endTop);
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
                mEndHandle.repositionWithViewport(context.viewport.left, context.viewport.top, context.zoomFactor);
            }
        });
    }
}
