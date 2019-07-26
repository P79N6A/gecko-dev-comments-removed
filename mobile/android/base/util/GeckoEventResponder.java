








package org.mozilla.gecko.util;

import org.json.JSONObject;

public interface GeckoEventResponder extends GeckoEventListener {
    String getResponse(JSONObject response);
}
