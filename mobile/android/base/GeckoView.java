




package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.TypedArray;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class GeckoView extends LayerView
    implements GeckoEventListener, ContextGetter {

    private static final String DEFAULT_SHARED_PREFERENCES_FILE = "GeckoView";
    private static final String LOGTAG = "GeckoView";

    private ChromeDelegate mChromeDelegate;
    private ContentDelegate mContentDelegate;

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
        GeckoAppShell.registerEventListener("Content:StateChange", this);
        GeckoAppShell.registerEventListener("Content:LoadError", this);
        GeckoAppShell.registerEventListener("Content:PageShow", this);
        GeckoAppShell.registerEventListener("DOMTitleChanged", this);
        GeckoAppShell.registerEventListener("Link:Favicon", this);
        GeckoAppShell.registerEventListener("Prompt:Show", this);
        GeckoAppShell.registerEventListener("Prompt:ShowTop", this);

        ThreadUtils.setUiThread(Thread.currentThread(), new Handler());
        initializeView(GeckoAppShell.getEventDispatcher());

        if (GeckoThread.checkAndSetLaunchState(GeckoThread.LaunchState.Launching, GeckoThread.LaunchState.Launched)) {
            
            GeckoProfile profile = GeckoProfile.get(context).forceCreate();
            BrowserDB.initialize(profile.getName());

            GeckoAppShell.setLayerView(this);
            GeckoThread.createAndStart();
        } else if(GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            
            
            connectToGecko();
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

    


    public void handleMessage(final String event, final JSONObject message) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (event.equals("Gecko:Ready")) {
                        GeckoView.this.handleReady(message);
                    } else if (event.equals("Content:StateChange")) {
                        GeckoView.this.handleStateChange(message);
                    } else if (event.equals("Content:LoadError")) {
                        GeckoView.this.handleLoadError(message);
                    } else if (event.equals("Content:PageShow")) {
                        GeckoView.this.handlePageShow(message);
                    } else if (event.equals("DOMTitleChanged")) {
                        GeckoView.this.handleTitleChanged(message);
                    } else if (event.equals("Link:Favicon")) {
                        GeckoView.this.handleLinkFavicon(message);
                    } else if (event.equals("Prompt:Show") || event.equals("Prompt:ShowTop")) {
                        GeckoView.this.handlePrompt(message);
                    }
                } catch (Exception e) {
                    Log.w(LOGTAG, "handleMessage threw for " + event, e);
                }
            }
        });
    }

    private void connectToGecko() {
        GeckoThread.setLaunchState(GeckoThread.LaunchState.GeckoRunning);
        Tab selectedTab = Tabs.getInstance().getSelectedTab();
        if (selectedTab != null)
            Tabs.getInstance().notifyListeners(selectedTab, Tabs.TabEvents.SELECTED);
        geckoConnected();
        GeckoAppShell.setLayerClient(getLayerClient());
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Viewport:Flush", null));
    }

    private void handleReady(final JSONObject message) {
        connectToGecko();

        if (mChromeDelegate != null) {
            mChromeDelegate.onReady(this);
        }
    }

    private void handleStateChange(final JSONObject message) throws JSONException {
        int state = message.getInt("state");
        if ((state & GeckoAppShell.WPL_STATE_IS_NETWORK) != 0) {
            if ((state & GeckoAppShell.WPL_STATE_START) != 0) {
                if (mContentDelegate != null) {
                    int id = message.getInt("tabID");
                    mContentDelegate.onPageStart(this, new Browser(id), message.getString("uri"));
                }
            } else if ((state & GeckoAppShell.WPL_STATE_STOP) != 0) {
                if (mContentDelegate != null) {
                    int id = message.getInt("tabID");
                    mContentDelegate.onPageStop(this, new Browser(id), message.getBoolean("success"));
                }
            }
        }
    }

    private void handleLoadError(final JSONObject message) throws JSONException {
        if (mContentDelegate != null) {
            int id = message.getInt("tabID");
            mContentDelegate.onPageStop(GeckoView.this, new Browser(id), false);
        }
    }

    private void handlePageShow(final JSONObject message) throws JSONException {
        if (mContentDelegate != null) {
            int id = message.getInt("tabID");
            mContentDelegate.onPageShow(GeckoView.this, new Browser(id));
        }
    }

    private void handleTitleChanged(final JSONObject message) throws JSONException {
        if (mContentDelegate != null) {
            int id = message.getInt("tabID");
            mContentDelegate.onReceivedTitle(GeckoView.this, new Browser(id), message.getString("title"));
        }
    }

    private void handleLinkFavicon(final JSONObject message) throws JSONException {
        if (mContentDelegate != null) {
            int id = message.getInt("tabID");
            mContentDelegate.onReceivedFavicon(GeckoView.this, new Browser(id), message.getString("href"), message.getInt("size"));
        }
    }

    private void handlePrompt(final JSONObject message) throws JSONException {
        if (mChromeDelegate != null) {
            String hint = message.optString("hint");
            if ("alert".equals(hint)) {
                String text = message.optString("text");
                mChromeDelegate.onAlert(GeckoView.this, null, text, new PromptResult(message.optString("guid")));
            } else if ("confirm".equals(hint)) {
                String text = message.optString("text");
                mChromeDelegate.onConfirm(GeckoView.this, null, text, new PromptResult(message.optString("guid")));
            } else if ("prompt".equals(hint)) {
                String text = message.optString("text");
                String defaultValue = message.optString("textbox0");
                mChromeDelegate.onPrompt(GeckoView.this, null, text, defaultValue, new PromptResult(message.optString("guid")));
            } else if ("remotedebug".equals(hint)) {
                mChromeDelegate.onDebugRequest(GeckoView.this, new PromptResult(message.optString("guid")));
            }
        }
    }

    




    public void setChromeDelegate(ChromeDelegate chrome) {
        mChromeDelegate = chrome;
    }

    




    public void setContentDelegate(ContentDelegate content) {
        mContentDelegate = content;
    }

    public static void setGeckoInterface(final BaseGeckoInterface geckoInterface) {
        GeckoAppShell.setGeckoInterface(geckoInterface);
    }

    public static GeckoAppShell.GeckoInterface getGeckoInterface() {
        return GeckoAppShell.getGeckoInterface();
    }

    protected String getSharedPreferencesFile() {
        return DEFAULT_SHARED_PREFERENCES_FILE;
    }

    public SharedPreferences getSharedPreferences() {
        return getContext().getSharedPreferences(getSharedPreferencesFile(), 0);
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

    



    public class PromptResult {
        private final int RESULT_OK = 0;
        private final int RESULT_CANCEL = 1;

        private final String mGUID;

        public PromptResult(String guid) {
            mGUID = guid;
        }

        private JSONObject makeResult(int resultCode) {
            JSONObject result = new JSONObject();
            try {
                result.put("guid", mGUID);
                result.put("button", resultCode);
            } catch(JSONException ex) { }
            return result;
        }

        


        public void confirm() {
            JSONObject result = makeResult(RESULT_OK);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Prompt:Reply", result.toString()));
        }

        



        public void confirmWithValue(String value) {
            JSONObject result = makeResult(RESULT_OK);
            try {
                result.put("textbox0", value);
            } catch(JSONException ex) { }
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Prompt:Reply", result.toString()));
        }

        


        public void cancel() {
            JSONObject result = makeResult(RESULT_CANCEL);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Prompt:Reply", result.toString()));
        }
    }

    public interface ChromeDelegate {
        



        public void onReady(GeckoView view);

        







        public void onAlert(GeckoView view, GeckoView.Browser browser, String message, GeckoView.PromptResult result);
    
        







        public void onConfirm(GeckoView view, GeckoView.Browser browser, String message, GeckoView.PromptResult result);
    
        








        public void onPrompt(GeckoView view, GeckoView.Browser browser, String message, String defaultValue, GeckoView.PromptResult result);
    
        





        public void onDebugRequest(GeckoView view, GeckoView.PromptResult result);
    }

    public interface ContentDelegate {
        





        public void onPageStart(GeckoView view, GeckoView.Browser browser, String url);
    
        





        public void onPageStop(GeckoView view, GeckoView.Browser browser, boolean success);
    
        





        public void onPageShow(GeckoView view, GeckoView.Browser browser);
    
        






        public void onReceivedTitle(GeckoView view, GeckoView.Browser browser, String title);
    
        







        public void onReceivedFavicon(GeckoView view, GeckoView.Browser browser, String url, int size);
    }

}
