



package org.mozilla.gecko.util;

import org.json.JSONObject;

import android.util.Log;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

public final class EventDispatcher {
    private static final String LOGTAG = "GeckoEventDispatcher";

    private final Map<String, CopyOnWriteArrayList<GeckoEventListener>> mEventListeners
                  = new HashMap<String, CopyOnWriteArrayList<GeckoEventListener>>();

    public void registerEventListener(String event, GeckoEventListener listener) {
        synchronized (mEventListeners) {
            CopyOnWriteArrayList<GeckoEventListener> listeners = mEventListeners.get(event);
            if (listeners == null) {
                
                
                
                listeners = new CopyOnWriteArrayList<GeckoEventListener>();
            } else if (listeners.contains(listener)) {
                Log.w(LOGTAG, "EventListener already registered for event '" + event + "'",
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
                Log.w(LOGTAG, "unregisterEventListener: event '" + event + "' has no listeners");
                return;
            }
            if (!listeners.remove(listener)) {
                Log.w(LOGTAG, "unregisterEventListener: tried to remove an unregistered listener " +
                              "for event '" + event + "'");
            }
            if (listeners.size() == 0) {
                mEventListeners.remove(event);
            }
        }
    }

    public String dispatchEvent(String message) {
        
        
        
        
        try {
            JSONObject json = new JSONObject(message);
            JSONObject gecko = json.has("gecko") ? json.getJSONObject("gecko") : null;
            if (gecko != null) {
                json = gecko;
            }

            String type = json.getString("type");

            if (gecko != null) {
                Log.w(LOGTAG, "Message '" + type + "' has deprecated 'gecko' property!");
            }

            CopyOnWriteArrayList<GeckoEventListener> listeners;
            synchronized (mEventListeners) {
                listeners = mEventListeners.get(type);
            }

            if (listeners == null || listeners.size() == 0) {
                Log.d(LOGTAG, "dispatchEvent: no listeners registered for event '" + type + "'");
                return "";
            }

            String response = null;

            for (GeckoEventListener listener : listeners) {
                listener.handleMessage(type, json);
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
