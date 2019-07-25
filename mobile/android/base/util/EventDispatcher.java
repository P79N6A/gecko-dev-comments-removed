



package org.mozilla.gecko.util;

import org.json.JSONObject;

import android.util.Log;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

public final class EventDispatcher {
    private static final String LOGTAG = "EventDispatcher";

    private final Map<String, CopyOnWriteArrayList<GeckoEventListener>> mEventListeners
                  = new HashMap<String, CopyOnWriteArrayList<GeckoEventListener>>();

    public void registerEventListener(String event, GeckoEventListener listener) {
        synchronized (mEventListeners) {
            CopyOnWriteArrayList<GeckoEventListener> listeners = mEventListeners.get(event);
            if (listeners == null) {
                
                
                
                listeners = new CopyOnWriteArrayList<GeckoEventListener>();
            } else if (listeners.contains(listener)) {
                Log.w(LOGTAG, "EventListener already registered for event \"" + event + "\"",
                      new IllegalArgumentException());
            }
            listeners.add(listener);
            mEventListeners.put(event, listeners);
        }
    }

    public void unregisterEventListener(String event, GeckoEventListener listener) {
        synchronized (mEventListeners) {
            CopyOnWriteArrayList<GeckoEventListener> listeners = mEventListeners.get(event);
            if (listeners == null) {
                return;
            }
            listeners.remove(listener);
            if (listeners.size() == 0) {
                mEventListeners.remove(event);
            }
        }
    }

    public String dispatchEvent(String message) {
        
        
        
        
        
        try {
            JSONObject json = new JSONObject(message);
            final JSONObject geckoObject = json.getJSONObject("gecko");
            String type = geckoObject.getString("type");

            CopyOnWriteArrayList<GeckoEventListener> listeners;
            synchronized (mEventListeners) {
                listeners = mEventListeners.get(type);
            }

            if (listeners == null)
                return "";

            String response = null;

            for (GeckoEventListener listener : listeners) {
                listener.handleMessage(type, geckoObject);
                if (listener instanceof GeckoEventResponder) {
                    String newResponse = ((GeckoEventResponder)listener).getResponse();
                    if (response != null && newResponse != null) {
                        Log.e(LOGTAG, "Received two responses for message of type " + type);
                    }
                    response = newResponse;
                }
            }

            if (response != null)
                return response;

        } catch (Exception e) {
            Log.e(LOGTAG, "handleGeckoMessage throws " + e, e);
        }

        return "";
    }
}
