




package org.mozilla.gecko;

import org.json.JSONObject;
import org.mozilla.gecko.util.EventCallback;


interface GeckoMediaPlayer {
    JSONObject toJSON();
    void load(String title, String url, String type, EventCallback callback);
    void play(EventCallback callback);
    void pause(EventCallback callback);
    void stop(EventCallback callback);
    void start(EventCallback callback);
    void end(EventCallback callback);
    void mirror(EventCallback callback);
    void message(String message, EventCallback callback);
}
