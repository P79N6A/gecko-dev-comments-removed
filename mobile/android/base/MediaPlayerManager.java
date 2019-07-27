



package org.mozilla.gecko;

import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import android.content.Context;
import android.support.v7.media.MediaControlIntent;
import android.support.v7.media.MediaRouteSelector;
import android.support.v7.media.MediaRouter;
import android.support.v7.media.MediaRouter.RouteInfo;
import android.util.Log;

import java.util.HashMap;


interface GeckoMediaPlayer {
    public JSONObject toJSON();
    public void load(String title, String url, String type, EventCallback callback);
    public void play(EventCallback callback);
    public void pause(EventCallback callback);
    public void stop(EventCallback callback);
    public void start(EventCallback callback);
    public void end(EventCallback callback);
}




class MediaPlayerManager implements NativeEventListener,
                                      GeckoAppShell.AppStateListener {
    private static final String LOGTAG = "GeckoMediaPlayerManager";

    private static final boolean SHOW_DEBUG = false;
    
    private static void debug(String msg, Exception e) {
        if (SHOW_DEBUG) {
            Log.e(LOGTAG, msg, e);
        }
    }

    private static void debug(String msg) {
        if (SHOW_DEBUG) {
            Log.d(LOGTAG, msg);
        }
    }

    private final Context context;
    private final MediaRouter mediaRouter;
    private final HashMap<String, GeckoMediaPlayer> displays = new HashMap<String, GeckoMediaPlayer>();
    private static MediaPlayerManager instance;

    public static void init(Context context) {
        if (instance != null) {
            debug("MediaPlayerManager initialized twice");
        }

        instance = new MediaPlayerManager(context);
    }

    private MediaPlayerManager(Context context) {
        this.context = context;

        if (context instanceof GeckoApp) {
            GeckoApp app = (GeckoApp) context;
            app.addAppStateListener(this);
        }

        mediaRouter = MediaRouter.getInstance(context); 
        EventDispatcher.getInstance().registerGeckoThreadListener(this, "MediaPlayer:Load",
                                                                        "MediaPlayer:Start",
                                                                        "MediaPlayer:Stop",
                                                                        "MediaPlayer:Play",
                                                                        "MediaPlayer:Pause",
                                                                        "MediaPlayer:Get",
                                                                        "MediaPlayer:End");
    }

    public static void onDestroy() {
        if (instance == null) {
            return;
        }

        EventDispatcher.getInstance().unregisterGeckoThreadListener(instance, "MediaPlayer:Load",
                                                                              "MediaPlayer:Start",
                                                                              "MediaPlayer:Stop",
                                                                              "MediaPlayer:Play",
                                                                              "MediaPlayer:Pause",
                                                                              "MediaPlayer:Get",
                                                                              "MediaPlayer:End");
        if (instance.context instanceof GeckoApp) {
            GeckoApp app = (GeckoApp) instance.context;
            app.removeAppStateListener(instance);
        }
    }

    
    @Override
    public void handleMessage(String event, final NativeJSObject message, final EventCallback callback) {
        debug(event);

        if ("MediaPlayer:Get".equals(event)) {
            final JSONObject result = new JSONObject();
            final JSONArray disps = new JSONArray();
            for (GeckoMediaPlayer disp : displays.values()) {
                disps.put(disp.toJSON());
            }

            try {
                result.put("displays", disps);
            } catch(JSONException ex) {
                Log.i(LOGTAG, "Error sending displays", ex);
            }

            callback.sendSuccess(result);
            return;
        }

        final GeckoMediaPlayer display = displays.get(message.getString("id"));
        if (display == null) {
            Log.e(LOGTAG, "Couldn't find a display for this id");
            callback.sendError(null);
            return;
        }

        if ("MediaPlayer:Play".equals(event)) {
            display.play(callback);
        } else if ("MediaPlayer:Start".equals(event)) {
            display.start(callback);
        } else if ("MediaPlayer:Stop".equals(event)) {
            display.stop(callback);
        } else if ("MediaPlayer:Pause".equals(event)) {
            display.pause(callback);
        } else if ("MediaPlayer:End".equals(event)) {
            display.end(callback);
        } else if ("MediaPlayer:Load".equals(event)) {
            final String url = message.optString("source", "");
            final String type = message.optString("type", "video/mp4");
            final String title = message.optString("title", "");
            display.load(title, url, type, callback);
        }
    }

    private final MediaRouter.Callback callback = new MediaRouter.Callback() {
        @Override
        public void onRouteRemoved(MediaRouter router, RouteInfo route) {
            debug("onRouteRemoved: route=" + route);
            displays.remove(route.getId());
        }

        @SuppressWarnings("unused")
        public void onRouteSelected(MediaRouter router, int type, MediaRouter.RouteInfo route) {
        }

        
        @SuppressWarnings("unused")
        public void onRouteUnselected(MediaRouter router, int type, RouteInfo route) {
        }

        @Override
        public void onRoutePresentationDisplayChanged(MediaRouter router, RouteInfo route) {
        }

        @Override
        public void onRouteVolumeChanged(MediaRouter router, RouteInfo route) {
        }

        @Override
        public void onRouteAdded(MediaRouter router, MediaRouter.RouteInfo route) {
            debug("onRouteAdded: route=" + route);
            GeckoMediaPlayer display = getMediaPlayerForRoute(route);
            if (display != null) {
                displays.put(route.getId(), display);
            }
        }

        @Override
        public void onRouteChanged(MediaRouter router, MediaRouter.RouteInfo route) {
            debug("onRouteChanged: route=" + route);
            GeckoMediaPlayer display = displays.get(route.getId());
            if (display != null) {
                displays.put(route.getId(), display);
            }
        }
    };

    private GeckoMediaPlayer getMediaPlayerForRoute(MediaRouter.RouteInfo route) {
        return null;
    }

    
    @Override
    public void onPause() {
        mediaRouter.removeCallback(callback);
    }

    @Override
    public void onResume() {
        MediaRouteSelector selectorBuilder = new MediaRouteSelector.Builder()
            .addControlCategory(MediaControlIntent.CATEGORY_LIVE_VIDEO)
            .addControlCategory(MediaControlIntent.CATEGORY_REMOTE_PLAYBACK)
            .build();
        mediaRouter.addCallback(selectorBuilder, callback, MediaRouter.CALLBACK_FLAG_REQUEST_DISCOVERY);
    }

    @Override
    public void onOrientationChanged() { }

}
