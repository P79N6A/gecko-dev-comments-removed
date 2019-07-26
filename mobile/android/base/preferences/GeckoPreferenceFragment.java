




package org.mozilla.gecko.preferences;

import org.mozilla.gecko.R;
import org.mozilla.gecko.PrefsHelper;

import android.preference.Preference;
import android.preference.PreferenceActivity;
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
        String resourceName = getArguments().getString("resource");

        int res = 0;
        if (resourceName != null) {
            
            res = getActivity().getResources().getIdentifier(resourceName,
                                                             "xml",
                                                             getActivity().getPackageName());
        }

        if (res == 0) {
            
            Log.e(LOGTAG, "Failed to find resource: " + resourceName + ". Displaying default settings.");

            boolean isMultiPane = ((PreferenceActivity) getActivity()).onIsMultiPane();
            res = isMultiPane ? R.xml.preferences_customize_tablet : R.xml.preferences;
        }
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
