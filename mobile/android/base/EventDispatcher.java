



package org.mozilla.gecko;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONObject;

import android.util.Log;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

public final class EventDispatcher {
    private static final String LOGTAG = "GeckoEventDispatcher";
    private static final String GUID = "__guid__";
    private static final String SUFFIX_RETURN = "Return";
    private static final String SUFFIX_ERROR = "Error";

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

    public void dispatchEvent(String message) {
        try {
            JSONObject json = new JSONObject(message);
            dispatchEvent(json);
        } catch (Exception e) {
            Log.e(LOGTAG, "dispatchEvent: malformed JSON.", e);
        }
    }

    public void dispatchEvent(JSONObject json) {
        
        
        
        
        try {
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
                return;
            }

            for (GeckoEventListener listener : listeners) {
                listener.handleMessage(type, json);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "handleGeckoMessage throws " + e, e);
        }

    }

    public static void sendResponse(JSONObject message, Object response) {
        sendResponseHelper(SUFFIX_RETURN, message, response);
    }

    public static void sendError(JSONObject message, Object response) {
        sendResponseHelper(SUFFIX_ERROR, message, response);
    }

    private static void sendResponseHelper(String suffix, JSONObject message, Object response) {
        try {
            final JSONObject wrapper = new JSONObject();
            wrapper.put(GUID, message.getString(GUID));
            wrapper.put("response", response);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(message.getString("type") + ":" + suffix, wrapper.toString()));
        } catch (Exception e) {
            Log.e(LOGTAG, "Unable to send " + suffix, e);
        }
    }
}
