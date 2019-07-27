




package org.mozilla.javaaddons.test;

import android.content.Context;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.javaaddons.JavaAddonInterfaceV1.EventCallback;
import org.mozilla.javaaddons.JavaAddonInterfaceV1.EventDispatcher;
import org.mozilla.javaaddons.JavaAddonInterfaceV1.EventListener;
import org.mozilla.javaaddons.JavaAddonInterfaceV1.RequestCallback;

public class JavaAddonV1 implements EventListener, RequestCallback {
    protected final EventDispatcher mDispatcher;

    public JavaAddonV1(Context context, EventDispatcher dispatcher) {
        mDispatcher = dispatcher;
        mDispatcher.registerEventListener(this, "JavaAddon:V1");
    }

    @Override
    public void handleMessage(Context context, String event, JSONObject message, EventCallback callback) {
        Log.i("JavaAddon", "handleMessage: " + event + ", " + message.toString());
        final JSONObject output = new JSONObject();
        try {
            output.put("outputStringKey", "inputStringKey=" + message.getString("inputStringKey"));
            output.put("outputIntKey", 1 + message.getInt("inputIntKey"));
        } catch (JSONException e) {
            
        }
        
        if (callback != null) {
            callback.sendSuccess(output);
        }

        
        final JSONObject input = new JSONObject();
        try {
            input.put("inputStringKey", "raw");
            input.put("inputIntKey", 3);
        } catch (JSONException e) {
            
        }
        mDispatcher.sendRequestToGecko("JavaAddon:V1:Request", input, this);
    }

    @Override
    public void onResponse(Context context, JSONObject jsonObject) {
        Log.i("JavaAddon", "onResponse: " + jsonObject.toString());
        
        
        mDispatcher.unregisterEventListener(this);
        mDispatcher.sendRequestToGecko("JavaAddon:V1:VerificationRequest", jsonObject, null);
    }
}
