




package org.mozilla.gecko;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.media.MediaControlIntent;
import android.support.v7.media.MediaRouteSelector;
import android.support.v7.media.MediaRouter;
import android.support.v7.media.MediaRouter.RouteInfo;
import android.util.Log;
import com.google.android.gms.cast.CastMediaControlIntent;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.mozglue.JNITarget;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;




public class MediaPlayerManager extends Fragment implements NativeEventListener {
    



    @JNITarget
    public static MediaPlayerManager newInstance() {
        return new MediaPlayerManager();
    }

    private static final String LOGTAG = "GeckoMediaPlayerManager";

    @JNITarget
    public static final String MEDIA_PLAYER_TAG = "MPManagerFragment";

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

    private MediaRouter mediaRouter = null;
    private final Map<String, GeckoMediaPlayer> displays = new HashMap<String, GeckoMediaPlayer>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EventDispatcher.getInstance().registerGeckoThreadListener(this,
                "MediaPlayer:Load",
                "MediaPlayer:Start",
                "MediaPlayer:Stop",
                "MediaPlayer:Play",
                "MediaPlayer:Pause",
                "MediaPlayer:End",
                "MediaPlayer:Mirror",
                "MediaPlayer:Message");
    }

    @Override
    @JNITarget
    public void onDestroy() {
        super.onDestroy();
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this,
                                                                    "MediaPlayer:Load",
                                                                    "MediaPlayer:Start",
                                                                    "MediaPlayer:Stop",
                                                                    "MediaPlayer:Play",
                                                                    "MediaPlayer:Pause",
                                                                    "MediaPlayer:End",
                                                                    "MediaPlayer:Mirror",
                                                                    "MediaPlayer:Message");
    }

    
    @Override
    public void handleMessage(String event, final NativeJSObject message, final EventCallback callback) {
        debug(event);

        final GeckoMediaPlayer display = displays.get(message.getString("id"));
        if (display == null) {
            Log.e(LOGTAG, "Couldn't find a display for this id: " + message.getString("id") + " for message: " + event);
            if (callback != null) {
                callback.sendError(null);
            }
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
        } else if ("MediaPlayer:Mirror".equals(event)) {
            display.mirror(callback);
        } else if ("MediaPlayer:Message".equals(event) && message.has("data")) {
            display.message(message.getString("data"), callback);
        } else if ("MediaPlayer:Load".equals(event)) {
            final String url = message.optString("source", "");
            final String type = message.optString("type", "video/mp4");
            final String title = message.optString("title", "");
            display.load(title, url, type, callback);
        }
    }

    private final MediaRouter.Callback callback =
        new MediaRouter.Callback() {
            @Override
            public void onRouteRemoved(MediaRouter router, RouteInfo route) {
                debug("onRouteRemoved: route=" + route);
                displays.remove(route.getId());
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(
                        "MediaPlayer:Removed", route.getId()));
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
                final GeckoMediaPlayer display = getMediaPlayerForRoute(route);
                if (display != null) {
                    displays.put(route.getId(), display);
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(
                            "MediaPlayer:Added", display.toJSON().toString()));
                }
            }

            @Override
            public void onRouteChanged(MediaRouter router, MediaRouter.RouteInfo route) {
                debug("onRouteChanged: route=" + route);
                final GeckoMediaPlayer display = displays.get(route.getId());
                if (display != null) {
                    displays.put(route.getId(), display);
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(
                            "MediaPlayer:Changed", display.toJSON().toString()));
                }
            }
        };

    private GeckoMediaPlayer getMediaPlayerForRoute(MediaRouter.RouteInfo route) {
        try {
            if (route.supportsControlCategory(MediaControlIntent.CATEGORY_REMOTE_PLAYBACK)) {
                return new ChromeCast(getActivity(), route);
            }
        } catch(Exception ex) {
            debug("Error handling presentation", ex);
        }

        return null;
    }

    @Override
    public void onPause() {
        super.onPause();
        mediaRouter.removeCallback(callback);
        mediaRouter = null;
    }

    @Override
    public void onResume() {
        super.onResume();

        
        if (mediaRouter != null) {
            return;
        }

        mediaRouter = MediaRouter.getInstance(getActivity());
        final MediaRouteSelector selectorBuilder = new MediaRouteSelector.Builder()
            .addControlCategory(MediaControlIntent.CATEGORY_LIVE_VIDEO)
            .addControlCategory(MediaControlIntent.CATEGORY_REMOTE_PLAYBACK)
            .addControlCategory(CastMediaControlIntent.categoryForCast(ChromeCast.MIRROR_RECEIVER_APP_ID))
            .build();
        mediaRouter.addCallback(selectorBuilder, callback, MediaRouter.CALLBACK_FLAG_REQUEST_DISCOVERY);
    }
}
