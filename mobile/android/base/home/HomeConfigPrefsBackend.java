




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.HomeConfig.HomeConfigBackend;
import org.mozilla.gecko.home.HomeConfig.OnChangeListener;
import org.mozilla.gecko.home.HomeConfig.PanelConfig;
import org.mozilla.gecko.home.HomeConfig.PanelType;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;

import static org.mozilla.gecko.home.HomeConfig.createBuiltinPanelConfig;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.Locale;

class HomeConfigPrefsBackend implements HomeConfigBackend {
    private static final String LOGTAG = "GeckoHomeConfigBackend";

    private static final String PREFS_CONFIG_KEY = "home_panels";
    private static final String PREFS_LOCALE_KEY = "home_locale";

    private final Context mContext;
    private PrefsListener mPrefsListener;
    private OnChangeListener mChangeListener;

    public HomeConfigPrefsBackend(Context context) {
        mContext = context;
    }

    private SharedPreferences getSharedPreferences() {
        return PreferenceManager.getDefaultSharedPreferences(mContext);
    }

    private List<PanelConfig> loadDefaultConfig() {
        final ArrayList<PanelConfig> panelConfigs = new ArrayList<PanelConfig>();

        panelConfigs.add(createBuiltinPanelConfig(mContext, PanelType.TOP_SITES,
                                                  EnumSet.of(PanelConfig.Flags.DEFAULT_PANEL)));

        panelConfigs.add(createBuiltinPanelConfig(mContext, PanelType.BOOKMARKS));

        
        
        if (!HardwareUtils.isLowMemoryPlatform()) {
            panelConfigs.add(createBuiltinPanelConfig(mContext, PanelType.READING_LIST));
        }

        final PanelConfig historyEntry = createBuiltinPanelConfig(mContext, PanelType.HISTORY);

        
        
        if (HardwareUtils.isTablet()) {
            panelConfigs.add(historyEntry);
        } else {
            panelConfigs.add(0, historyEntry);
        }

        return panelConfigs;
    }

    private List<PanelConfig> loadConfigFromString(String jsonString) {
        final JSONArray jsonPanelConfigs;
        try {
            jsonPanelConfigs = new JSONArray(jsonString);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error loading the list of home panels from JSON prefs", e);

            
            return loadDefaultConfig();
        }

        final ArrayList<PanelConfig> panelConfigs = new ArrayList<PanelConfig>();

        final int count = jsonPanelConfigs.length();
        for (int i = 0; i < count; i++) {
            try {
                final JSONObject jsonPanelConfig = jsonPanelConfigs.getJSONObject(i);
                final PanelConfig panelConfig = new PanelConfig(jsonPanelConfig);
                panelConfigs.add(panelConfig);
            } catch (Exception e) {
                Log.e(LOGTAG, "Exception loading PanelConfig from JSON", e);
            }
        }

        return panelConfigs;
    }

    @Override
    public List<PanelConfig> load() {
        final SharedPreferences prefs = getSharedPreferences();
        final String jsonString = prefs.getString(PREFS_CONFIG_KEY, null);

        final List<PanelConfig> panelConfigs;
        if (TextUtils.isEmpty(jsonString)) {
            panelConfigs = loadDefaultConfig();
        } else {
            panelConfigs = loadConfigFromString(jsonString);
        }

        return panelConfigs;
    }

    @Override
    public void save(List<PanelConfig> panelConfigs) {
        final JSONArray jsonPanelConfigs = new JSONArray();

        final int count = panelConfigs.size();
        for (int i = 0; i < count; i++) {
            try {
                final PanelConfig panelConfig = panelConfigs.get(i);
                final JSONObject jsonPanelConfig = panelConfig.toJSON();
                jsonPanelConfigs.put(jsonPanelConfig);
            } catch (Exception e) {
                Log.e(LOGTAG, "Exception converting PanelConfig to JSON", e);
            }
        }

        final SharedPreferences prefs = getSharedPreferences();
        final SharedPreferences.Editor editor = prefs.edit();

        final String jsonString = jsonPanelConfigs.toString();
        editor.putString(PREFS_CONFIG_KEY, jsonString);
        editor.putString(PREFS_LOCALE_KEY, Locale.getDefault().toString());
        editor.commit();
    }

    @Override
    public String getLocale() {
        final SharedPreferences prefs = getSharedPreferences();

        String locale = prefs.getString(PREFS_LOCALE_KEY, null);
        if (locale == null) {
            
            final String currentLocale = Locale.getDefault().toString();

            final SharedPreferences.Editor editor = prefs.edit();
            editor.putString(PREFS_LOCALE_KEY, currentLocale);
            editor.commit();

            
            
            
            
            if (!prefs.contains(PREFS_CONFIG_KEY)) {
                locale = currentLocale;
            }
        }

        return locale;
    }

    @Override
    public void setOnChangeListener(OnChangeListener listener) {
        final SharedPreferences prefs = getSharedPreferences();

        if (mChangeListener != null) {
            prefs.unregisterOnSharedPreferenceChangeListener(mPrefsListener);
            mPrefsListener = null;
        }

        mChangeListener = listener;

        if (mChangeListener != null) {
            mPrefsListener = new PrefsListener();
            prefs.registerOnSharedPreferenceChangeListener(mPrefsListener);
        }
    }

    private class PrefsListener implements OnSharedPreferenceChangeListener {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            if (TextUtils.equals(key, PREFS_CONFIG_KEY)) {
                mChangeListener.onChange();
            }
        }
    }
}
