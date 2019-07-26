



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

    public void setDrawListener(GeckoLayerClient.DrawListener listener) {
        GeckoAppShell.getLayerView().getLayerClient().setDrawListener(listener);
    }
}
