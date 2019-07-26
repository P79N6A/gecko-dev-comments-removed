



package org.mozilla.gecko;

import org.mozilla.gecko.gfx.GeckoLayerClient;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.GeckoEventListener;

import android.app.Activity;
import android.database.Cursor;
import android.view.View;

import java.nio.IntBuffer;
import java.util.List;



















@RobocopTarget
public class RobocopAPI {
    private final GeckoApp mGeckoApp;

    public RobocopAPI(Activity activity) {
        mGeckoApp = (GeckoApp)activity;
    }

    public void registerEventListener(String event, GeckoEventListener listener) {
        GeckoAppShell.registerEventListener(event, listener);
    }

    public void unregisterEventListener(String event, GeckoEventListener listener) {
        GeckoAppShell.unregisterEventListener(event, listener);
    }

    public void preferencesGetEvent(int requestId, String[] prefNames) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createPreferencesGetEvent(requestId, prefNames));
    }

    public void preferencesObserveEvent(int requestId, String[] prefNames) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createPreferencesObserveEvent(requestId, prefNames));
    }

    public void preferencesRemoveObserversEvent(int requestId) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createPreferencesRemoveObserversEvent(requestId));
    }

    public void setDrawListener(GeckoLayerClient.DrawListener listener) {
        GeckoAppShell.getLayerView().getLayerClient().setDrawListener(listener);
    }
}
