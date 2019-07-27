



package org.mozilla.gecko;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSContainer;
import org.mozilla.gecko.util.NativeJSObject;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

@RobocopTarget
public final class EventDispatcher {
    private static final String LOGTAG = "GeckoEventDispatcher";
    private static final String GUID = "__guid__";
    private static final String STATUS_ERROR = "error";
    private static final String STATUS_SUCCESS = "success";

    private static final EventDispatcher INSTANCE = new EventDispatcher();

    





    private static final int GECKO_NATIVE_EVENTS_COUNT = 0; 
    private static final int GECKO_JSON_EVENTS_COUNT = 256; 

    private final Map<String, List<NativeEventListener>> mGeckoThreadNativeListeners =
        new HashMap<String, List<NativeEventListener>>(GECKO_NATIVE_EVENTS_COUNT);
    private final Map<String, List<GeckoEventListener>> mGeckoThreadJSONListeners =
        new HashMap<String, List<GeckoEventListener>>(GECKO_JSON_EVENTS_COUNT);

    public static EventDispatcher getInstance() {
        return INSTANCE;
    }

    private EventDispatcher() {
    }

    private <T> void registerListener(final Class<? extends List<T>> listType,
                                      final Map<String, List<T>> listenersMap,
                                      final T listener,
                                      final String[] events) {
        try {
            synchronized (listenersMap) {
                for (final String event : events) {
                    List<T> listeners = listenersMap.get(event);
                    if (listeners == null) {
                        listeners = listType.newInstance();
                        listenersMap.put(event, listeners);
                    }
                    if (!AppConstants.RELEASE_BUILD && listeners.contains(listener)) {
                        throw new IllegalStateException("Already registered " + event);
                    }
                    listeners.add(listener);
                }
            }
        } catch (final IllegalAccessException e) {
            throw new IllegalArgumentException("Invalid new list type", e);
        } catch (final InstantiationException e) {
            throw new IllegalArgumentException("Invalid new list type", e);
        }
    }

    private <T> void checkNotRegistered(final Map<String, List<T>> listenersMap,
                                        final String[] events) {
        synchronized (listenersMap) {
            for (final String event: events) {
                if (listenersMap.get(event) != null) {
                    throw new IllegalStateException(
                        "Already registered " + event + " under a different type");
                }
            }
        }
    }

    private <T> void unregisterListener(final Map<String, List<T>> listenersMap,
                                        final T listener,
                                        final String[] events) {
        synchronized (listenersMap) {
            for (final String event : events) {
                List<T> listeners = listenersMap.get(event);
                if ((listeners == null ||
                     !listeners.remove(listener)) && !AppConstants.RELEASE_BUILD) {
                    throw new IllegalArgumentException(event + " was not registered");
                }
            }
        }
    }

    @SuppressWarnings("unchecked")
    public void registerGeckoThreadListener(final NativeEventListener listener,
                                            final String... events) {
        checkNotRegistered(mGeckoThreadJSONListeners, events);

        
        
        
        
        
        registerListener((Class)CopyOnWriteArrayList.class,
                         mGeckoThreadNativeListeners, listener, events);
    }

    @Deprecated 
    @SuppressWarnings("unchecked")
    public void registerGeckoThreadListener(final GeckoEventListener listener,
                                            final String... events) {
        checkNotRegistered(mGeckoThreadNativeListeners, events);

        registerListener((Class)CopyOnWriteArrayList.class,
                         mGeckoThreadJSONListeners, listener, events);
    }

    public void unregisterGeckoThreadListener(final NativeEventListener listener,
                                              final String... events) {
        unregisterListener(mGeckoThreadNativeListeners, listener, events);
    }

    @Deprecated 
    public void unregisterGeckoThreadListener(final GeckoEventListener listener,
                                              final String... events) {
        unregisterListener(mGeckoThreadJSONListeners, listener, events);
    }

    public void dispatchEvent(final NativeJSContainer message) {
        
        final String type = message.optString("type", null);
        if (type == null) {
            Log.e(LOGTAG, "JSON message must have a type property");
            return;
        }

        final List<NativeEventListener> listeners;
        synchronized (mGeckoThreadNativeListeners) {
            listeners = mGeckoThreadNativeListeners.get(type);
        }

        final String guid = message.optString(GUID, null);
        EventCallback callback = null;
        if (guid != null) {
            callback = new GeckoEventCallback(guid, type);
        }

        if (listeners != null) {
            if (listeners.size() == 0) {
                Log.w(LOGTAG, "No listeners for " + type);
            }
            try {
                for (final NativeEventListener listener : listeners) {
                    listener.handleMessage(type, message, callback);
                }
            } catch (final NativeJSObject.InvalidPropertyException e) {
                Log.e(LOGTAG, "Exception occurred while handling " + type, e);
            }
            
            
            return;
        }

        try {
            
            dispatchEvent(new JSONObject(message.toString()), callback);
        } catch (final JSONException e) {
            Log.e(LOGTAG, "Cannot parse JSON", e);
        } catch (final UnsupportedOperationException e) {
            Log.e(LOGTAG, "Cannot convert message to JSON", e);
        }
    }

    public void dispatchEvent(final JSONObject message, final EventCallback callback) {
        
        
        
        
        try {
            final String type = message.getString("type");

            List<GeckoEventListener> listeners;
            synchronized (mGeckoThreadJSONListeners) {
                listeners = mGeckoThreadJSONListeners.get(type);
            }
            if (listeners == null || listeners.size() == 0) {
                Log.w(LOGTAG, "No listeners for " + type);

                
                if (callback != null) {
                    callback.sendError("No listeners for request");
                }
                return;
            }
            for (final GeckoEventListener listener : listeners) {
                listener.handleMessage(type, message);
            }
        } catch (final JSONException e) {
            Log.e(LOGTAG, "handleGeckoMessage throws " + e, e);
        }
    }

    @RobocopTarget
    @Deprecated
    public static void sendResponse(JSONObject message, Object response) {
        sendResponseHelper(STATUS_SUCCESS, message, response);
    }

    @Deprecated
    public static void sendError(JSONObject message, Object response) {
        sendResponseHelper(STATUS_ERROR, message, response);
    }

    @Deprecated
    private static void sendResponseHelper(String status, JSONObject message, Object response) {
        try {
            final JSONObject wrapper = new JSONObject();
            wrapper.put(GUID, message.getString(GUID));
            wrapper.put("status", status);
            wrapper.put("response", response);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(
                    message.getString("type") + ":Response", wrapper.toString()));
        } catch (final JSONException e) {
            Log.e(LOGTAG, "Unable to send response", e);
        }
    }

    private static class GeckoEventCallback implements EventCallback {
        private final String guid;
        private final String type;
        private boolean sent;

        public GeckoEventCallback(final String guid, final String type) {
            this.guid = guid;
            this.type = type;
        }

        public void sendSuccess(final Object response) {
            sendResponse(STATUS_SUCCESS, response);
        }

        public void sendError(final Object response) {
            sendResponse(STATUS_ERROR, response);
        }

        private void sendResponse(final String status, final Object response) {
            if (sent) {
                throw new IllegalStateException("Callback has already been executed for type=" +
                        type + ", guid=" + guid);
            }

            sent = true;

            try {
                final JSONObject wrapper = new JSONObject();
                wrapper.put(GUID, guid);
                wrapper.put("status", status);
                wrapper.put("response", response);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(type + ":Response",
                        wrapper.toString()));
            } catch (final JSONException e) {
                Log.e(LOGTAG, "Unable to send response for: " + type, e);
            }
        }
    }
}
