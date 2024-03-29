




package org.mozilla.gecko;

import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.util.Log;




public final class OrderedBroadcastHelper
             implements GeckoEventListener
{
    public static final String LOGTAG = "GeckoOrdBroadcast";

    public static final String SEND_EVENT = "OrderedBroadcast:Send";

    protected final Context mContext;

    public OrderedBroadcastHelper(Context context) {
        mContext = context;

        EventDispatcher dispatcher = EventDispatcher.getInstance();
        if (dispatcher == null) {
            Log.e(LOGTAG, "Gecko event dispatcher must not be null", new RuntimeException());
            return;
        }
        dispatcher.registerGeckoThreadListener(this, SEND_EVENT);
    }

    public synchronized void uninit() {
        EventDispatcher dispatcher = EventDispatcher.getInstance();
        if (dispatcher == null) {
            Log.e(LOGTAG, "Gecko event dispatcher must not be null", new RuntimeException());
            return;
        }
        dispatcher.unregisterGeckoThreadListener(this, SEND_EVENT);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        if (!SEND_EVENT.equals(event)) {
            Log.e(LOGTAG, "OrderedBroadcastHelper got unexpected message " + event);
            return;
        }

        try {
            final String action = message.getString("action");
            if (action == null) {
                Log.e(LOGTAG, "action must not be null");
                return;
            }

            final String responseEvent = message.getString("responseEvent");
            if (responseEvent == null) {
                Log.e(LOGTAG, "responseEvent must not be null");
                return;
            }

            
            final JSONObject token = (message.has("token") && !message.isNull("token")) ?
                message.getJSONObject("token") : null;

            
            
            
            final String permission = message.has("permission") ?
                                      (message.isNull("permission") ? null : message.getString("permission")) :
                                      GlobalConstants.PER_ANDROID_PACKAGE_PERMISSION;

            final BroadcastReceiver resultReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    int code = getResultCode();

                    if (code == Activity.RESULT_OK) {
                        String data = getResultData();

                        JSONObject res = new JSONObject();
                        try {
                            res.put("action", action);
                            res.put("token", token);
                            res.put("data", data);
                        } catch (JSONException e) {
                            Log.e(LOGTAG, "Got exception in onReceive handling action " + action, e);
                            return;
                        }

                        GeckoEvent event = GeckoEvent.createBroadcastEvent(responseEvent, res.toString());
                        GeckoAppShell.sendEventToGecko(event);
                    }
                }
            };

            Intent intent = new Intent(action);
            
            
            if (token != null && token.has("data")) {
                intent.putExtra("token", token.getString("data"));
            }

            mContext.sendOrderedBroadcast(intent,
                                          permission,
                                          resultReceiver,
                                          null,
                                          Activity.RESULT_OK,
                                          null,
                                          null);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Got exception in handleMessage handling event " + event, e);
            return;
        }
    }
}
