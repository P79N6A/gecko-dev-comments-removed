



package org.mozilla.gecko.preferences;

import android.content.Context;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.util.AttributeSet;
import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.util.GeckoEventListener;

public class SearchPreferenceCategory extends PreferenceCategory implements GeckoEventListener {
    public static final String LOGTAG = "SearchPrefCategory";

    private SearchEnginePreference mDefaultEngineReference;

    
    

    public SearchPreferenceCategory(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public SearchPreferenceCategory(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public SearchPreferenceCategory(Context context) {
        super(context);
    }

    @Override
    protected void onAttachedToActivity() {
        super.onAttachedToActivity();

        
        setOrderingAsAdded(true);

        
        GeckoAppShell.registerEventListener("SearchEngines:Data", this);
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("SearchEngines:Get", null));
    }

    @Override
    public void handleMessage(String event, final JSONObject data) {
        if (event.equals("SearchEngines:Data")) {
            
            
            GeckoAppShell.unregisterEventListener("SearchEngines:Data", this);

            
            JSONArray engines;
            try {
                engines = data.getJSONArray("searchEngines");
            } catch (JSONException e) {
                Log.e(LOGTAG, "Unable to decode search engine data from Gecko.", e);
                return;
            }

            
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
                        
                        
                        
                        enginePreference.setIsDefaultEngine(true);
                        mDefaultEngineReference = enginePreference;
                    }
                } catch (JSONException e) {
                    Log.e(LOGTAG, "JSONException parsing engine at index " + i, e);
                }
            }
        }
    }

    



    private void setFallbackDefaultEngine() {
        if (getPreferenceCount() > 0) {
            SearchEnginePreference aEngine = (SearchEnginePreference) getPreference(0);
            setDefault(aEngine);
        }
    }

    




    private void sendGeckoEngineEvent(String event, SearchEnginePreference engine) {
        JSONObject json = new JSONObject();
        try {
            json.put("engine", engine.getTitle());
        } catch (JSONException e) {
            Log.e(LOGTAG, "JSONException creating search engine configuration change message for Gecko.", e);
            return;
        }
        GeckoAppShell.notifyGeckoOfEvent(GeckoEvent.createBroadcastEvent(event, json.toString()));
    }

    

    



    public void uninstall(SearchEnginePreference engine) {
        removePreference(engine);
        if (engine == mDefaultEngineReference) {
            
            setFallbackDefaultEngine();
        }

        sendGeckoEngineEvent("SearchEngines:Remove", engine);
    }

    



    public void setDefault(SearchEnginePreference engine) {
        engine.setIsDefaultEngine(true);
        mDefaultEngineReference.setIsDefaultEngine(false);
        mDefaultEngineReference = engine;

        sendGeckoEngineEvent("SearchEngines:SetDefault", engine);
    }
}
