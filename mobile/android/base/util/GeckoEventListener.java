




package org.mozilla.gecko.util;

import org.json.JSONObject;




public interface GeckoEventListener {
    void handleMessage(String event, JSONObject message);
}
