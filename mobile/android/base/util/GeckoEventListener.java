




package org.mozilla.gecko.util;

import org.json.JSONObject;
import org.mozilla.gecko.mozglue.RobocopTarget;

@RobocopTarget
public interface GeckoEventListener {
    void handleMessage(String event, JSONObject message);
}
