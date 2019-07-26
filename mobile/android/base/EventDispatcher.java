



package org.mozilla.gecko;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSContainer;

import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

public final class EventDispatcher {
    private static final String LOGTAG = "GeckoEventDispatcher";
    private static final String GUID = "__guid__";
    private static final String STATUS_ERROR = "error";
    private static final String STATUS_SUCCESS = "success";

    





    private static final int GECKO_NATIVE_EVENTS_COUNT = 0; 
    private static final int GECKO_JSON_EVENTS_COUNT = 256; 

    private final Map<String, List<NativeEventListener>> mGeckoThreadNativeListeners =
        new HashMap<String, List<NativeEventListener>>(GECKO_NATIVE_EVENTS_COUNT);
    private final Map<String, List<GeckoEventListener>> mGeckoThreadJSONListeners =
        new HashMap<String, List<GeckoEventListener>>(GECKO_JSON_EVENTS_COUNT);

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
    private void registerGeckoThreadListener(final GeckoEventListener listener,
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
    private void unregisterGeckoThreadListener(final GeckoEventListener listener,
                                               final String... events) {
        unregisterListener(mGeckoThreadJSONListeners, listener, events);
    }

    @Deprecated 
    public void registerEventListener(final String event, final GeckoEventListener listener) {
        registerGeckoThreadListener(listener, event);
    }

    @Deprecated 
    public void unregisterEventListener(final String event, final GeckoEventListener listener) {
        unregisterGeckoThreadListener(listener, event);
    }

    public void dispatchEvent(final NativeJSContainer message) {
        try {
            
            final String type = message.getString("type");

            final List<NativeEventListener> listeners;
            synchronized (mGeckoThreadNativeListeners) {
                listeners = mGeckoThreadNativeListeners.get(type);
            }
            if (listeners != null) {
                if (listeners.size() == 0) {
                    Log.w(LOGTAG, "No listeners for " + type);
                }
                for (final NativeEventListener listener : listeners) {
                    listener.handleMessage(type, message);
                }
                
                
                return;
            }
        } catch (final IllegalArgumentException e) {
            
        }
        try {
            
            dispatchEvent(new JSONObject(message.toString()));
        } catch (final JSONException e) {
            Log.e(LOGTAG, "Cannot parse JSON");
        } catch (final UnsupportedOperationException e) {
            Log.e(LOGTAG, "Cannot convert message to JSON");
        }
    }

    public void dispatchEvent(final JSONObject message) {
        
        
        
        
        try {
            final String type = message.getString("type");

            List<GeckoEventListener> listeners;
            synchronized (mGeckoThreadJSONListeners) {
                listeners = mGeckoThreadJSONListeners.get(type);
            }
            if (listeners == null || listeners.size() == 0) {
                Log.w(LOGTAG, "No listeners for " + type);
                return;
            }
            for (final GeckoEventListener listener : listeners) {
                listener.handleMessage(type, message);
            }
        } catch (final JSONException e) {
            Log.e(LOGTAG, "handleGeckoMessage throws " + e, e);
        }
    }

    public static void sendResponse(JSONObject message, Object response) {
        sendResponseHelper(STATUS_SUCCESS, message, response);
    }

    public static void sendError(JSONObject message, Object response) {
        sendResponseHelper(STATUS_ERROR, message, response);
    }

    private static void sendResponseHelper(String status, JSONObject message, Object response) {
        try {
            final JSONObject wrapper = new JSONObject();
            wrapper.put(GUID, message.getString(GUID));
            wrapper.put("status", status);
            wrapper.put("response", response);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(message.getString("type") + ":Response", wrapper.toString()));
        } catch (JSONException e) {
            Log.e(LOGTAG, "Unable to send response", e);
        }
    }
}
