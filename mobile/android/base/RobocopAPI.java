



package org.mozilla.gecko;

import org.mozilla.gecko.gfx.GeckoLayerClient;
import org.mozilla.gecko.gfx.PanningPerfAPI;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.sqlite.SQLiteBridge;
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

    public void broadcastEvent(String subject, String data) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(subject, data));
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

    public Cursor querySql(String dbPath, String query) {
        GeckoLoader.loadSQLiteLibs(mGeckoApp, mGeckoApp.getApplication().getPackageResourcePath());
        return new SQLiteBridge(dbPath).rawQuery(query, null);
    }

    
    public static void startFrameTimeRecording() {
        PanningPerfAPI.startFrameTimeRecording();
    }

    public static List<Long> stopFrameTimeRecording() {
        return PanningPerfAPI.stopFrameTimeRecording();
    }

    public static void startCheckerboardRecording() {
        PanningPerfAPI.startCheckerboardRecording();
    }

    public static List<Float> stopCheckerboardRecording() {
        return PanningPerfAPI.stopCheckerboardRecording();
    }
}
