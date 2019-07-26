




package org.mozilla.gecko.home;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.util.Log;

import org.mozilla.gecko.GeckoSharedPrefs;









class PanelAuthCache {
    private static final String LOGTAG = "GeckoPanelAuthCache";

    
    private static final String PREFS_PANEL_AUTH_PREFIX = "home_panels_auth_";

    private final Context mContext;
    private SharedPrefsListener mSharedPrefsListener;
    private OnChangeListener mChangeListener;

    public interface OnChangeListener {
        public void onChange(String panelId, boolean isAuthenticated);
    }

    public PanelAuthCache(Context context) {
        mContext = context;
    }

    private SharedPreferences getSharedPreferences() {
        return GeckoSharedPrefs.forProfile(mContext);
    }

    private String getPanelAuthKey(String panelId) {
        return PREFS_PANEL_AUTH_PREFIX + panelId;
    }

    public boolean isAuthenticated(String panelId) {
        final SharedPreferences prefs = getSharedPreferences();
        return prefs.getBoolean(getPanelAuthKey(panelId), false);
    }

    public void setOnChangeListener(OnChangeListener listener) {
        final SharedPreferences prefs = getSharedPreferences();

        if (mChangeListener != null) {
            prefs.unregisterOnSharedPreferenceChangeListener(mSharedPrefsListener);
            mSharedPrefsListener = null;
        }

        mChangeListener = listener;

        if (mChangeListener != null) {
            mSharedPrefsListener = new SharedPrefsListener();
            prefs.registerOnSharedPreferenceChangeListener(mSharedPrefsListener);
        }
    }

    private class SharedPrefsListener implements OnSharedPreferenceChangeListener {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
            if (key.startsWith(PREFS_PANEL_AUTH_PREFIX)) {
                final String panelId = key.substring(PREFS_PANEL_AUTH_PREFIX.length());
                final boolean isAuthenticated = prefs.getBoolean(key, false);

                Log.d(LOGTAG, "Auth state changed: panelId=" + panelId + ", isAuthenticated=" + isAuthenticated);
                mChangeListener.onChange(panelId, isAuthenticated);
            }
        }
    }
}
