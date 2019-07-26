




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONObject;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.net.Uri;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.os.Handler;

public class GeckoView extends LayerView
    implements GeckoEventListener, ContextGetter {

    public GeckoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.GeckoView);
        String url = a.getString(R.styleable.GeckoView_url);
        boolean doInit = a.getBoolean(R.styleable.GeckoView_doinit, true);
        a.recycle();

        if (!doInit)
            return;

        
        
        if (!(context instanceof GeckoActivity)) {
            BrowserDB.setEnableContentProviders(false);
        }

        if (url != null) {
            GeckoThread.setUri(url);
            GeckoThread.setAction(Intent.ACTION_VIEW);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createURILoadEvent(url));
        }
        GeckoAppShell.setContextGetter(this);
        if (context instanceof Activity) {
            Tabs tabs = Tabs.getInstance();
            tabs.attachToContext(context);
        }
        GeckoAppShell.registerEventListener("Gecko:Ready", this);

        ThreadUtils.setUiThread(Thread.currentThread(), new Handler());
        initializeView(GeckoAppShell.getEventDispatcher());


        GeckoProfile profile = GeckoProfile.get(context);
        BrowserDB.initialize(profile.getName());

        if (GeckoThread.checkAndSetLaunchState(GeckoThread.LaunchState.Launching, GeckoThread.LaunchState.Launched)) {
            GeckoAppShell.setLayerView(this);
            GeckoThread.createAndStart();
        }
    }

    public void loadUrl(String uri) {
        Tabs.getInstance().loadUrl(uri);
    }

    public void loadUrlInNewTab(String uri) {
        Tabs.getInstance().loadUrl(uri, Tabs.LOADURL_NEW_TAB);
     }

    public void handleMessage(String event, JSONObject message) {
        if (event.equals("Gecko:Ready")) {
            GeckoThread.setLaunchState(GeckoThread.LaunchState.GeckoRunning);
            Tab selectedTab = Tabs.getInstance().getSelectedTab();
            if (selectedTab != null)
                Tabs.getInstance().notifyListeners(selectedTab, Tabs.TabEvents.SELECTED);
            geckoConnected();
            GeckoAppShell.setLayerClient(getLayerClient());
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Viewport:Flush", null));
            show();
            requestRender();
        }
    }

    public static void setGeckoInterface(GeckoAppShell.GeckoInterface aGeckoInterface) {
        GeckoAppShell.setGeckoInterface(aGeckoInterface);
    }
}
