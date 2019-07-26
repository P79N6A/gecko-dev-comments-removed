




package org.mozilla.gecko.home;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class PanelManager implements GeckoEventListener {
    private static final String LOGTAG = "GeckoPanelManager";

    public class PanelInfo {
        public final String id;
        public final String title;

        public PanelInfo(String id, String title) {
            this.id = id;
            this.title = title;
        }
    }

    private final Context mContext;

    public PanelManager(Context context) {
        mContext = context;

        
        GeckoAppShell.getEventDispatcher().registerEventListener("HomePanels:Added", this);
    }

    




    public List<PanelInfo> getPanelInfos() {
        final ArrayList<PanelInfo> panelInfos = new ArrayList<PanelInfo>();

        
        final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
        final String prefValue = prefs.getString("home_lists", "");

        if (!TextUtils.isEmpty(prefValue)) {
            try {
                final JSONArray lists = new JSONArray(prefValue);
                for (int i = 0; i < lists.length(); i++) {
                    final JSONObject list = lists.getJSONObject(i);
                    final PanelInfo info = new PanelInfo(list.getString("id"), list.getString("title"));
                    panelInfos.add(info);
                }
            } catch (JSONException e) {
                Log.e(LOGTAG, "Exception getting list info", e);
            }
        }
        return panelInfos;
    }

    


    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            final PanelInfo info = new PanelInfo(message.getString("id"), message.getString("title"));

            

        } catch (JSONException e) {
            Log.e(LOGTAG, "Exception handling " + event + " message", e);
        }
    }
}
