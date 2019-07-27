package org.mozilla.gecko.util;

import java.util.concurrent.atomic.AtomicInteger;

import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.mozglue.RobocopTarget;

import android.util.Log;

public abstract class GeckoRequest {
    private static final String LOGTAG = "GeckoRequest";
    private static final AtomicInteger currentId = new AtomicInteger(0);

    private final int id = currentId.getAndIncrement();
    private final String name;
    private final String data;

    








    @RobocopTarget
    public GeckoRequest(String name, Object data) {
        this.name = name;
        final JSONObject message = new JSONObject();
        try {
            message.put("id", id);
            message.put("data", data);
        } catch (JSONException e) {
            Log.e(LOGTAG, "JSON error", e);
        }
        this.data = message.toString();
    }

    




    public int getId() {
        return id;
    }

    




    public String getName() {
        return name;
    }

    




    public String getData() {
        return data;
    }

    




    @RobocopTarget
    public abstract void onResponse(NativeJSObject nativeJSObject);

    









    @RobocopTarget
    public void onError() {
        throw new RuntimeException("Unhandled error for GeckoRequest: " + name);
    }
}