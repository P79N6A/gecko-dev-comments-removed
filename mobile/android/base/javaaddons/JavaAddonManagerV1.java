




package org.mozilla.gecko.javaaddons;

import android.content.Context;
import android.support.v4.util.Pair;
import android.util.Log;
import dalvik.system.DexClassLoader;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.gecko.util.GeckoRequest;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.javaaddons.JavaAddonInterfaceV1;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.Map;

public class JavaAddonManagerV1 implements NativeEventListener {
    private static final String LOGTAG = "GeckoJavaAddonMgrV1";
    public static final String MESSAGE_LOAD = "JavaAddonManagerV1:Load";
    public static final String MESSAGE_UNLOAD = "JavaAddonManagerV1:Unload";

    private static JavaAddonManagerV1 sInstance;

    
    private Context mApplicationContext;

    private final org.mozilla.gecko.EventDispatcher mDispatcher;

    
    private final Map<String, EventDispatcherImpl> mGUIDToDispatcherMap = new HashMap<>();

    public static synchronized JavaAddonManagerV1 getInstance() {
        if (sInstance == null) {
            sInstance = new JavaAddonManagerV1();
        }
        return sInstance;
    }

    private JavaAddonManagerV1() {
        mDispatcher = org.mozilla.gecko.EventDispatcher.getInstance();
    }

    public synchronized void init(Context applicationContext) {
        if (mApplicationContext != null) {
            
            return;
        }
        mApplicationContext = applicationContext;
        mDispatcher.registerGeckoThreadListener(this,
                MESSAGE_LOAD,
                MESSAGE_UNLOAD);
    }

    protected String getExtension(String filename) {
        if (filename == null) {
            return "";
        }
        final int last = filename.lastIndexOf(".");
        if (last < 0) {
            return "";
        }
        return filename.substring(last);
    }

    protected synchronized EventDispatcherImpl registerNewInstance(String classname, String filename)
            throws ClassNotFoundException, InstantiationException, IllegalAccessException, InvocationTargetException, NoSuchMethodException, IOException {
        Log.d(LOGTAG, "Attempting to instantiate " + classname + "from filename " + filename);

        
        final String extension = getExtension(filename);
        final File dexFile = GeckoJarReader.extractStream(mApplicationContext, filename, mApplicationContext.getCacheDir(), "." + extension);
        try {
            if (dexFile == null) {
                throw new IOException("Could not find file " + filename);
            }
            final File tmpDir = mApplicationContext.getDir("dex", 0); 
            final DexClassLoader loader = new DexClassLoader(dexFile.getAbsolutePath(), tmpDir.getAbsolutePath(), null, mApplicationContext.getClassLoader());
            final Class<?> c = loader.loadClass(classname);
            final Constructor<?> constructor = c.getDeclaredConstructor(Context.class, JavaAddonInterfaceV1.EventDispatcher.class);
            final String guid = Utils.generateGuid();
            final EventDispatcherImpl dispatcher = new EventDispatcherImpl(guid, filename);
            final Object instance = constructor.newInstance(mApplicationContext, dispatcher);
            mGUIDToDispatcherMap.put(guid, dispatcher);
            return dispatcher;
        } finally {
            
            if (dexFile != null) {
                dexFile.delete();
            }
        }
    }

    @Override
    public synchronized void handleMessage(String event, NativeJSObject message, org.mozilla.gecko.util.EventCallback callback) {
        try {
            switch (event) {
                case MESSAGE_LOAD: {
                    if (callback == null) {
                        throw new IllegalArgumentException("callback must not be null");
                    }
                    final String classname = message.getString("classname");
                    final String filename = message.getString("filename");
                    final EventDispatcherImpl dispatcher = registerNewInstance(classname, filename);
                    callback.sendSuccess(dispatcher.guid);
                }
                break;
                case MESSAGE_UNLOAD: {
                    if (callback == null) {
                        throw new IllegalArgumentException("callback must not be null");
                    }
                    final String guid = message.getString("guid");
                    final EventDispatcherImpl dispatcher = mGUIDToDispatcherMap.remove(guid);
                    if (dispatcher == null) {
                        Log.w(LOGTAG, "Attempting to unload addon with unknown associated dispatcher; ignoring.");
                        callback.sendSuccess(false);
                    }
                    dispatcher.unregisterAllEventListeners();
                    callback.sendSuccess(true);
                }
                break;
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message [" + event + "]", e);
            if (callback != null) {
                callback.sendError("Exception handling message [" + event + "]: " + e.toString());
            }
        }
    }

    








    private class EventDispatcherImpl implements JavaAddonInterfaceV1.EventDispatcher {
        private final String guid;
        private final String dexFileName;

        
        private final Map<JavaAddonInterfaceV1.EventListener, Pair<NativeEventListener, String[]>> mListenerToWrapperMap = new IdentityHashMap<>();

        public EventDispatcherImpl(String guid, String dexFileName) {
            this.guid = guid;
            this.dexFileName = dexFileName;
        }

        protected class ListenerWrapper implements NativeEventListener {
            private final JavaAddonInterfaceV1.EventListener listener;

            public ListenerWrapper(JavaAddonInterfaceV1.EventListener listener) {
                this.listener = listener;
            }

            @Override
            public void handleMessage(String prefixedEvent, NativeJSObject message, final org.mozilla.gecko.util.EventCallback callback) {
                if (!prefixedEvent.startsWith(guid + ":")) {
                    return;
                }
                final String event = prefixedEvent.substring(guid.length() + 1); 
                try {
                    JavaAddonInterfaceV1.EventCallback callbackAdapter = null;
                    if (callback != null) {
                        callbackAdapter = new JavaAddonInterfaceV1.EventCallback() {
                            @Override
                            public void sendSuccess(Object response) {
                                callback.sendSuccess(response);
                            }

                            @Override
                            public void sendError(Object response) {
                                callback.sendError(response);
                            }
                        };
                    }
                    final JSONObject json = new JSONObject(message.toString());
                    listener.handleMessage(mApplicationContext, event, json, callbackAdapter);
                } catch (Exception e) {
                    Log.e(LOGTAG, "Exception handling message [" + prefixedEvent + "]", e);
                    if (callback != null) {
                        callback.sendError("Got exception handling message [" + prefixedEvent + "]: " + e.toString());
                    }
                }
            }
        }

        @Override
        public synchronized void registerEventListener(final JavaAddonInterfaceV1.EventListener listener, String... events) {
            if (mListenerToWrapperMap.containsKey(listener)) {
                Log.e(LOGTAG, "Attempting to register listener which is already registered; ignoring.");
                return;
            }

            final NativeEventListener listenerWrapper = new ListenerWrapper(listener);

            final String[] prefixedEvents = new String[events.length];
            for (int i = 0; i < events.length; i++) {
                prefixedEvents[i] = this.guid + ":" + events[i];
            }
            mDispatcher.registerGeckoThreadListener(listenerWrapper, prefixedEvents);
            mListenerToWrapperMap.put(listener, new Pair<>(listenerWrapper, prefixedEvents));
        }

        @Override
        public synchronized void unregisterEventListener(final JavaAddonInterfaceV1.EventListener listener) {
            final Pair<NativeEventListener, String[]> pair = mListenerToWrapperMap.remove(listener);
            if (pair == null) {
                Log.e(LOGTAG, "Attempting to unregister listener which is not registered; ignoring.");
                return;
            }
            mDispatcher.unregisterGeckoThreadListener(pair.first, pair.second);
        }


        protected synchronized void unregisterAllEventListeners() {
            
            for (Pair<NativeEventListener, String[]> pair : mListenerToWrapperMap.values()) {
                 mDispatcher.unregisterGeckoThreadListener(pair.first, pair.second);
            }
            mListenerToWrapperMap.clear();
        }

        @Override
        public void sendRequestToGecko(final String event, final JSONObject message, final JavaAddonInterfaceV1.RequestCallback callback) {
            final String prefixedEvent = guid + ":" + event;
            GeckoAppShell.sendRequestToGecko(new GeckoRequest(prefixedEvent, message) {
                @Override
                public void onResponse(NativeJSObject nativeJSObject) {
                    if (callback == null) {
                        
                        return;
                    }
                    try {
                        final JSONObject json = new JSONObject(nativeJSObject.toString());
                        callback.onResponse(GeckoAppShell.getContext(), json);
                    } catch (JSONException e) {
                        
                        Log.e(LOGTAG, "Exception handling response to request [" + event + "]:", e);
                    }
                }
            });
        }
    }
}
