




































package org.mozilla.gecko;

import org.json.JSONObject;

public interface GeckoEventListener {
    public void handleMessage(String event, JSONObject message);
}
