



package org.mozilla.gecko.preferences;

import android.content.Context;
import android.preference.Preference;
import android.util.AttributeSet;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.GeckoEventListener;

public class SearchPreferenceCategory extends CustomListCategory implements GeckoEventListener {
    public static final String LOGTAG = "SearchPrefCategory";

    public SearchPreferenceCategory(Context context) {
        super(context);
    }

    public SearchPreferenceCategory(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public SearchPreferenceCategory(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onAttachedToActivity() {
        super.onAttachedToActivity();

        
        GeckoAppShell.registerEventListener("SearchEngines:Data", this);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:GetVisible", null));
    }

    @Override
    protected void onPrepareForRemoval() {
        GeckoAppShell.unregisterEventListener("SearchEngines:Data", this);
    }

    @Override
    public void setDefault(CustomListPreference item) {
        super.setDefault(item);

        sendGeckoEngineEvent("SearchEngines:SetDefault", item.getTitle().toString());
    }

    @Override
    public void uninstall(CustomListPreference item) {
        super.uninstall(item);

        sendGeckoEngineEvent("SearchEngines:Remove", item.getTitle().toString());
    }

    @Override
    public void handleMessage(String event, final JSONObject data) {
        if (event.equals("SearchEngines:Data")) {
            
            JSONArray engines;
            try {
                engines = data.getJSONArray("searchEngines");
            } catch (JSONException e) {
                Log.e(LOGTAG, "Unable to decode search engine data from Gecko.", e);
                return;
            }

            
            this.removeAll();

            
            for (int i = 0; i < engines.length(); i++) {
                try {
                    JSONObject engineJSON = engines.getJSONObject(i);
                    final String engineName = engineJSON.getString("name");

                    SearchEnginePreference enginePreference = new SearchEnginePreference(getContext(), this);
                    enginePreference.setSearchEngineFromJSON(engineJSON);
                    enginePreference.setOnPreferenceClickListener(new OnPreferenceClickListener() {
                        @Override
                        public boolean onPreferenceClick(Preference preference) {
                            SearchEnginePreference sPref = (SearchEnginePreference) preference;
                            
                            sPref.showDialog();
                            return true;
                        }
                    });

                    addPreference(enginePreference);

                    
                    if (i == 0) {
                        
                        
                        
                        enginePreference.setIsDefault(true);
                        mDefaultReference = enginePreference;
                    }
                } catch (JSONException e) {
                    Log.e(LOGTAG, "JSONException parsing engine at index " + i, e);
                }
            }
        }
    }

    




    private void sendGeckoEngineEvent(String event, String engineName) {
        JSONObject json = new JSONObject();
        try {
            json.put("engine", engineName);
        } catch (JSONException e) {
            Log.e(LOGTAG, "JSONException creating search engine configuration change message for Gecko.", e);
            return;
        }
        GeckoAppShell.notifyGeckoOfEvent(GeckoEvent.createBroadcastEvent(event, json.toString()));
    }
}
