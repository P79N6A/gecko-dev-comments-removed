




































package org.mozilla.gecko;

import java.util.ArrayList;

import android.os.Bundle;
import android.content.res.Resources;
import android.content.Context;
import android.preference.*;
import android.preference.Preference.*;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import org.json.*;

public class GeckoPreferences
    extends PreferenceActivity
    implements OnPreferenceChangeListener
{
    private static final String LOGTAG = "GeckoPreferences";

    private static Context sContext;
    private static JSONArray sJSONPrefs = null;
    private ArrayList<String> mPreferencesList = new ArrayList<String>();
    private PreferenceScreen mPreferenceScreen;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sContext = this;
        addPreferencesFromResource(R.xml.preferences);
        mPreferenceScreen = getPreferenceScreen();
        initGroups(mPreferenceScreen);

        
        if (sJSONPrefs != null)
            refresh();
    }

    private void initGroups(PreferenceGroup preferences) {
        final int count = preferences.getPreferenceCount();
        for (int i = 0; i < count; i++) {
            Preference pref = preferences.getPreference(i);
            if (pref instanceof PreferenceGroup)
                initGroups((PreferenceGroup)pref);
            else {
                pref.setOnPreferenceChangeListener(this);
                mPreferencesList.add(pref.getKey());
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String prefName = preference.getKey();
        setPreference(prefName, newValue);
        if (preference instanceof ListPreference)
            ((ListPreference)preference).setSummary((String)newValue);
        return true;
    }

    public static void setData(JSONArray jsonPrefs) {
        sJSONPrefs = jsonPrefs;
    }

    public static boolean isLoaded() {
        return sJSONPrefs != null;
    }

    
    private void refresh() {
        try {
            
            final String[] homepageValues = sContext.getResources().getStringArray(R.array.pref_homepage_values);
            final Preference homepagePref = mPreferenceScreen.findPreference("browser.startup.homepage");
            GeckoAppShell.getMainHandler().post(new Runnable() {
                public void run() {
                    Tab tab = Tabs.getInstance().getSelectedTab();
                    homepageValues[2] = tab.getURL();
                    ((ListPreference)homepagePref).setEntryValues(homepageValues);
                }
            });

            final int length = sJSONPrefs.length();
            for (int i = 0; i < length; i++) {
                JSONObject jPref = sJSONPrefs.getJSONObject(i);
                final String prefName = jPref.getString("name");
                final String prefType = jPref.getString("type");
                final Preference pref = mPreferenceScreen.findPreference(prefName);

                if (prefName.equals("browser.startup.homepage")) {
                    final String value = jPref.getString("value");
                    GeckoAppShell.getMainHandler().post(new Runnable() {
                        public void run() {
                            pref.setSummary(value);
                        }
                    });
                }

                if (pref instanceof CheckBoxPreference && "bool".equals(prefType)) {
                    final boolean value = jPref.getBoolean("value");
                    GeckoAppShell.getMainHandler().post(new Runnable() {
                        public void run() {
                            if (((CheckBoxPreference)pref).isChecked() != value)
                                ((CheckBoxPreference)pref).setChecked(value);
                        }
                    });
                } else if (pref instanceof EditTextPreference && "string".equals(prefType)) {
                    final String value = jPref.getString("value");
                    GeckoAppShell.getMainHandler().post(new Runnable() {
                        public void run() {
                            ((EditTextPreference)pref).setText(value);
                        }
                    });
                } else if (pref instanceof ListPreference && "string".equals(prefType)) {
                    final String value = jPref.getString("value");
                    GeckoAppShell.getMainHandler().post(new Runnable() {
                        public void run() {
                            ((ListPreference)pref).setValue(value);
                        }
                    });
                }
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Problem parsing preferences response: ", e);
        }
    }

    public static void setPreference(String pref, Object value) {
        
        JSONObject jsonPref = null;
        try {
            for (int i = 0; i < sJSONPrefs.length(); i++) {
                if (sJSONPrefs.getJSONObject(i).getString("name").equals(pref)) {
                    jsonPref = sJSONPrefs.getJSONObject(i);
                    if (value instanceof Boolean)
                        jsonPref.put("value", ((Boolean)value).booleanValue());
                    else if (value instanceof Integer)
                        jsonPref.put("value", ((Integer)value).intValue());
                    else
                        jsonPref.put("value", String.valueOf(value));
                    break;
                }
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "JSON exception: ", e);
            return;
        }

        if (jsonPref == null) {
            Log.e(LOGTAG, "invalid preference given to setPreference()");
            return;
        }

        
        GeckoEvent event = new GeckoEvent("Preferences:Set", jsonPref.toString());
        GeckoAppShell.sendEventToGecko(event);
    }
}
