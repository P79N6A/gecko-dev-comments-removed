




package org.mozilla.gecko.home;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public class PanelManager implements GeckoEventListener {
    private static final String LOGTAG = "GeckoPanelManager";

    public class PanelInfo {
        private final String mId;
        private final String mTitle;
        private final JSONObject mJSONData;

        public PanelInfo(String id, String title, JSONObject jsonData) {
            mId = id;
            mTitle = title;
            mJSONData = jsonData;
        }

        public String getId() {
            return mId;
        }

        public String getTitle() {
            return mTitle;
        }

        public PanelConfig toPanelConfig() {
            try {
                return new PanelConfig(mJSONData);
            } catch (Exception e) {
                Log.e(LOGTAG, "Failed to convert PanelInfo to PanelConfig", e);
                return null;
            }
        }
    }

    public interface RequestCallback {
        public void onComplete(List<PanelInfo> panelInfos);
    }

    private static AtomicInteger sRequestId = new AtomicInteger(0);

    
    private static final SparseArray<RequestCallback> sCallbacks = new SparseArray<RequestCallback>();

    




    public void requestAvailablePanels(RequestCallback callback) {
        final int requestId = sRequestId.getAndIncrement();

        synchronized(sCallbacks) {
            
            if (sCallbacks.size() == 0) {
                GeckoAppShell.getEventDispatcher().registerEventListener("HomePanels:Data", this);
            }
            sCallbacks.put(requestId, callback);
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("HomePanels:Get", Integer.toString(requestId)));
    }

    


    @Override
    public void handleMessage(String event, JSONObject message) {
        final ArrayList<PanelInfo> panelInfos = new ArrayList<PanelInfo>();

        try {
            final JSONArray panels = message.getJSONArray("panels");
            final int count = panels.length();
            for (int i = 0; i < count; i++) {
                final PanelInfo panelInfo = getPanelInfoFromJSON(panels.getJSONObject(i));
                panelInfos.add(panelInfo);
            }

            final RequestCallback callback;
            final int requestId = message.getInt("requestId");

            synchronized(sCallbacks) {
                callback = sCallbacks.get(requestId);
                sCallbacks.delete(requestId);

                
                if (sCallbacks.size() == 0) {
                    GeckoAppShell.getEventDispatcher().unregisterEventListener("HomePanels:Data", this);
                }
            }

            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    callback.onComplete(panelInfos);
                }
            });
        } catch (JSONException e) {
            Log.e(LOGTAG, "Exception handling " + event + " message", e);
        }
    }

    private PanelInfo getPanelInfoFromJSON(JSONObject jsonPanelInfo) throws JSONException {
        final String id = jsonPanelInfo.getString("id");
        final String title = jsonPanelInfo.getString("title");

        return new PanelInfo(id, title, jsonPanelInfo);
    }
}
