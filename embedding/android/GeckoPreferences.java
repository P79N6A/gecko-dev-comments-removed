




































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
    private static final String LOG_FILE_NAME = "GeckoPreferences";
    private ArrayList<String> mPreferencesList = new ArrayList<String>();
    private static PreferenceScreen mPreferenceScreen;
    private static Context sContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sContext = this;
        addPreferencesFromResource(R.xml.preferences);
        mPreferenceScreen = getPreferenceScreen();
        initGroups(mPreferenceScreen);
        initValues();
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
    protected void onDestroy() {
        super.onDestroy();
        mPreferenceScreen = null;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String prefName = preference.getKey();
        setPreference(prefName, newValue);
        ((ListPreference)preference).setSummary((String)newValue);
        return true;
    }

    
    private void initValues() {
        JSONArray jsonPrefs = new JSONArray(mPreferencesList);

        GeckoEvent event = new GeckoEvent("Preferences:Get", jsonPrefs.toString());
        GeckoAppShell.sendEventToGecko(event);
    }

    
    public static void refresh(JSONArray jsonPrefs) {
        try {
            if (mPreferenceScreen == null)
                return;

            final String[] homepageValues = sContext.getResources().getStringArray(R.array.pref_homepage_values);

            
            Tab tab = Tabs.getInstance().getSelectedTab();
            String currentUrl = tab.getURL();
            final Preference homepagePref = mPreferenceScreen.findPreference("browser.startup.homepage");
            homepageValues[2] = currentUrl;
            GeckoAppShell.getMainHandler().post(new Runnable() {
                public void run() {
                    ((ListPreference)homepagePref).setEntryValues(homepageValues);
                }
            });

            final int length = jsonPrefs.length();
            for (int i = 0; i < length; i++) {
                JSONObject jPref = jsonPrefs.getJSONObject(i);
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
            Log.e(LOG_FILE_NAME, "Problem parsing preferences response: ", e);
        }
    }

    
    public static void setPreference(String pref, Object value) {
        try {
            JSONObject jsonPref = new JSONObject();
            jsonPref.put("name", pref);
            if (value instanceof Boolean) {
                jsonPref.put("type", "bool");
                jsonPref.put("value", ((Boolean)value).booleanValue());
            }
            else if (value instanceof Integer) {
                jsonPref.put("type", "int");
                jsonPref.put("value", ((Integer)value).intValue());
            }
            else {
                jsonPref.put("type", "string");
                jsonPref.put("value", String.valueOf(value));
            }

            GeckoEvent event = new GeckoEvent("Preferences:Set", jsonPref.toString());
            GeckoAppShell.sendEventToGecko(event);
        } catch (JSONException e) {
            Log.e(LOG_FILE_NAME, "JSON exception: ", e);
        }
    }
}
