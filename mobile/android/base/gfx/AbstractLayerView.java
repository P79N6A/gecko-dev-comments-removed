




































package org.mozilla.gecko.gfx;

import android.content.Context;
import android.widget.RelativeLayout;
import android.view.View;

import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.InputConnectionHandler;
import org.mozilla.gecko.GeckoInputConnection;

public interface AbstractLayerView {
    public LayerController getController();
    public GeckoInputConnection setInputConnectionHandler();
    public View getAndroidView();
    
    public void setViewportSize(IntSize size);
    public void requestRender();
    public boolean post(Runnable action);
    public boolean postDelayed(Runnable action, long delayMillis);
    public Context getContext();
    public int getMaxTextureSize();
    public void clearEventQueue();
    public void processEventQueue();
}

