




package org.mozilla.gecko;

import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.os.Bundle;
import android.util.Log;





public class GeckoPreferenceFragment extends PreferenceFragment {

    private static final String LOGTAG = "GeckoPreferenceFragment";
    private int mPrefsRequestId = 0;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        String resource = getArguments().getString("resource");
        int res = getActivity().getResources().getIdentifier(resource,
                                                             "xml",
                                                             getActivity().getPackageName());
        addPreferencesFromResource(res);

        PreferenceScreen screen = getPreferenceScreen();
        setPreferenceScreen(screen);
        mPrefsRequestId = ((GeckoPreferences)getActivity()).setupPreferences(screen);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mPrefsRequestId > 0) {
            PrefsHelper.removeObserver(mPrefsRequestId);
        }
    }
}
