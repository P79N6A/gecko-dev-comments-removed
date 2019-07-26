




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.HardwareUtils;
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
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class GeckoView extends LayerView
    implements GeckoEventListener, ContextGetter {

    private static final String LOGTAG = "GeckoView";

    public GeckoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.GeckoView);
        String url = a.getString(R.styleable.GeckoView_url);
        boolean doInit = a.getBoolean(R.styleable.GeckoView_doinit, true);
        a.recycle();

        if (!doInit)
            return;

        
        
        boolean isGeckoActivity = false;
        try {
            isGeckoActivity = context instanceof GeckoActivity;
        } catch (NoClassDefFoundError ex) {}

        if (!isGeckoActivity) {
            
            
            if (context instanceof Activity && getGeckoInterface() == null) {
                setGeckoInterface(new BaseGeckoInterface(context));
            }

            Clipboard.init(context);
            HardwareUtils.init(context);
            GeckoNetworkManager.getInstance().init(context);

            GeckoLoader.loadMozGlue();
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

        GeckoProfile profile = GeckoProfile.get(context).forceCreate();
        BrowserDB.initialize(profile.getName());

        if (GeckoThread.checkAndSetLaunchState(GeckoThread.LaunchState.Launching, GeckoThread.LaunchState.Launched)) {
            GeckoAppShell.setLayerView(this);
            GeckoThread.createAndStart();
        }
    }

    



    public Browser addBrowser(String url) {
        Tab tab = Tabs.getInstance().loadUrl(url, Tabs.LOADURL_NEW_TAB);
        if (tab != null) {
            return new Browser(tab.getId());
        }
        return null;
    }

    



    public void removeBrowser(Browser browser) {
        Tab tab = Tabs.getInstance().getTab(browser.getId());
        if (tab != null) {
            Tabs.getInstance().closeTab(tab);
        }
    }

    



    public void setCurrentBrowser(Browser browser) {
        Tab tab = Tabs.getInstance().getTab(browser.getId());
        if (tab != null) {
            Tabs.getInstance().selectTab(tab.getId());
        }
    }

    



    public Browser getCurrentBrowser() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab != null) {
            return new Browser(tab.getId());
        }
        return null;
    }

    



    public List<Browser> getBrowsers() {
        ArrayList<Browser> browsers = new ArrayList<Browser>();
        Iterable<Tab> tabs = Tabs.getInstance().getTabsInOrder();
        for (Tab tab : tabs) {
            browsers.add(new Browser(tab.getId()));
        }
        return Collections.unmodifiableList(browsers);
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

    public static void setGeckoInterface(final BaseGeckoInterface geckoInterface) {
        GeckoAppShell.setGeckoInterface(geckoInterface);
    }

    public static GeckoAppShell.GeckoInterface getGeckoInterface() {
        return GeckoAppShell.getGeckoInterface();
    }

    



    public class Browser {
        private final int mId;
        private Browser(int Id) {
            mId = Id;
        }

        




        private int getId() {
            return mId;
        }

        



        public void loadUrl(String url) {
            JSONObject args = new JSONObject();
            try {
                args.put("url", url);
                args.put("parentId", -1);
                args.put("newTab", false);
                args.put("tabID", mId);
            } catch (Exception e) {
                Log.w(LOGTAG, "Error building JSON arguments for loadUrl.", e);
            }
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tab:Load", args.toString()));
        }

        



        public void reload() {
            Tab tab = Tabs.getInstance().getTab(mId);
            if (tab != null) {
                tab.doReload();
            }
        }

        


        public void stop() {
            Tab tab = Tabs.getInstance().getTab(mId);
            if (tab != null) {
                tab.doStop();
            }
        }

        





        public boolean canGoBack() {
            Tab tab = Tabs.getInstance().getTab(mId);
            if (tab != null) {
                return tab.canDoBack();
            }
            return false;
        }

        


        public void goBack() {
            Tab tab = Tabs.getInstance().getTab(mId);
            if (tab != null) {
                tab.doBack();
            }
        }

        





        public boolean canGoForward() {
            Tab tab = Tabs.getInstance().getTab(mId);
            if (tab != null) {
                return tab.canDoForward();
            }
            return false;
        }

        


        public void goForward() {
            Tab tab = Tabs.getInstance().getTab(mId);
            if (tab != null) {
                tab.doForward();
            }
        }
    }
}
