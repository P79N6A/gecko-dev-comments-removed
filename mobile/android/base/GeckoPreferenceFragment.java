




package org.mozilla.gecko;

import java.util.ArrayList;

import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.os.Bundle;
import android.util.Log;





public class GeckoPreferenceFragment extends PreferenceFragment {

    private static final String LOGTAG = "GeckoPreferenceFragment";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        String resource = getArguments().getString("resource");
        int res = getActivity().getResources().getIdentifier(resource,
                                                             "xml",
                                                             getActivity().getPackageName());
        addPreferencesFromResource(res);

        

        PreferenceScreen screen = stripCategories(getPreferenceScreen());
        setPreferenceScreen(screen);
        ((GeckoPreferences)getActivity()).setupPreferences(screen);
    }

    private PreferenceScreen stripCategories(PreferenceScreen preferenceScreen) {
        PreferenceScreen newScreen = getPreferenceManager().createPreferenceScreen(preferenceScreen.getContext());
        int order = 0;
        if (preferenceScreen.getPreferenceCount() > 0 && preferenceScreen.getPreference(0) instanceof PreferenceCategory) {
            PreferenceCategory cat = (PreferenceCategory) preferenceScreen.getPreference(0);
            for (int i = 0; i < cat.getPreferenceCount(); i++) {
                Preference pref = cat.getPreference(i);
                pref.setOrder(order++);
                newScreen.addPreference(pref);
            }
        }

        for (int i = 1; i < preferenceScreen.getPreferenceCount(); i++) {
            Preference pref = preferenceScreen.getPreference(i);
            pref.setOrder(order++);
            newScreen.addPreference(pref);
        }

        return newScreen;
    }
}
