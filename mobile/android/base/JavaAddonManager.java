




package org.mozilla.gecko;

import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.GeckoEventResponder;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import dalvik.system.DexClassLoader;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.lang.reflect.Constructor;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
























class JavaAddonManager implements GeckoEventListener {
    private static final String LOGTAG = "GeckoJavaAddonManager";

    private static JavaAddonManager sInstance;

    private final EventDispatcher mDispatcher;
    private final Map<String, Map<String, GeckoEventListener>> mAddonCallbacks;

    private Context mApplicationContext;

    public static JavaAddonManager getInstance() {
        if (sInstance == null) {
            sInstance = new JavaAddonManager();
        }
        return sInstance;
    }

    private JavaAddonManager() {
        mDispatcher = GeckoAppShell.getEventDispatcher();
        mAddonCallbacks = new HashMap<String, Map<String, GeckoEventListener>>();
    }

    void init(Context applicationContext) {
        mApplicationContext = applicationContext;
        mDispatcher.registerEventListener("Dex:Load", this);
        mDispatcher.registerEventListener("Dex:Unload", this);
    }

    public void handleMessage(String event, JSONObject message) {
        try {
            if (event.equals("Dex:Load")) {
                String zipFile = message.getString("zipfile");
                String implClass = message.getString("impl");
                Log.d(LOGTAG, "Attempting to load classes.dex file from " + zipFile + " and instantiate " + implClass);
                try {
                    File tmpDir = mApplicationContext.getDir("dex", 0);
                    DexClassLoader loader = new DexClassLoader(zipFile, tmpDir.getAbsolutePath(), null, ClassLoader.getSystemClassLoader());
                    Class<?> c = loader.loadClass(implClass);
                    try {
                        Constructor<?> constructor = c.getDeclaredConstructor(Map.class);
                        Map<String, Handler.Callback> callbacks = new HashMap<String, Handler.Callback>();
                        constructor.newInstance(callbacks);
                        registerCallbacks(zipFile, callbacks);
                    } catch (NoSuchMethodException nsme) {
                        Log.d(LOGTAG, "Did not find constructor with parameters Map<String, Handler.Callback>. Falling back to default constructor...");
                        
                        c.newInstance();
                    }
                } catch (Exception e) {
                    Log.e(LOGTAG, "Unable to load dex successfully", e);
                }
            } else if (event.equals("Dex:Unload")) {
                String zipFile = message.getString("zipfile");
                unregisterCallbacks(zipFile);
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Exception handling message [" + event + "]:", e);
        }
    }

    private void registerCallbacks(String zipFile, Map<String, Handler.Callback> callbacks) {
        Map<String, GeckoEventListener> addonCallbacks = mAddonCallbacks.get(zipFile);
        if (addonCallbacks != null) {
            Log.w(LOGTAG, "Found pre-existing callbacks for zipfile [" + zipFile + "]; aborting re-registration!");
            return;
        }
        addonCallbacks = new HashMap<String, GeckoEventListener>();
        for (String event : callbacks.keySet()) {
            CallbackWrapper wrapper = new CallbackWrapper(callbacks.get(event));
            mDispatcher.registerEventListener(event, wrapper);
            addonCallbacks.put(event, wrapper);
        }
        mAddonCallbacks.put(zipFile, addonCallbacks);
    }

    private void unregisterCallbacks(String zipFile) {
        Map<String, GeckoEventListener> callbacks = mAddonCallbacks.remove(zipFile);
        if (callbacks == null) {
            Log.w(LOGTAG, "Attempting to unregister callbacks from zipfile [" + zipFile + "] which has no callbacks registered.");
            return;
        }
        for (String event : callbacks.keySet()) {
            mDispatcher.unregisterEventListener(event, callbacks.get(event));
        }
    }

    private static class CallbackWrapper implements GeckoEventResponder {
        private final Handler.Callback mDelegate;
        private Bundle mBundle;

        CallbackWrapper(Handler.Callback delegate) {
            mDelegate = delegate;
        }

        private Bundle jsonToBundle(JSONObject json) {
            
            
            Bundle b = new Bundle();
            for (Iterator keys = json.keys(); keys.hasNext(); ) {
                try {
                    String key = (String)keys.next();
                    Object value = json.get(key);
                    if (value instanceof Integer) {
                        b.putInt(key, (Integer)value);
                    } else if (value instanceof String) {
                        b.putString(key, (String)value);
                    } else if (value instanceof Boolean) {
                        b.putBoolean(key, (Boolean)value);
                    } else if (value instanceof Long) {
                        b.putLong(key, (Long)value);
                    } else if (value instanceof Double) {
                        b.putDouble(key, (Double)value);
                    }
                } catch (JSONException e) {
                    Log.d(LOGTAG, "Error during JSON->bundle conversion", e);
                }
            }
            return b;
        }

        public void handleMessage(String event, JSONObject json) {
            try {
                if (mBundle != null) {
                    Log.w(LOGTAG, "Event [" + event + "] handler is re-entrant; response messages may be lost");
                }
                mBundle = jsonToBundle(json);
                Message msg = new Message();
                msg.setData(mBundle);
                mDelegate.handleMessage(msg);
            } catch (Exception e) {
                Log.e(LOGTAG, "Caught exception thrown from wrapped addon message handler", e);
            }
        }

        public String getResponse() {
            String response = mBundle.getString("response");
            mBundle = null;
            return response;
        }
    }
}
