




package org.mozilla.gecko.preferences;

import java.lang.reflect.Field;

import org.mozilla.gecko.R;
import org.mozilla.gecko.PrefsHelper;

import android.app.Activity;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.ViewConfiguration;





public class GeckoPreferenceFragment extends PreferenceFragment {

    private static final String LOGTAG = "GeckoPreferenceFragment";
    private int mPrefsRequestId = 0;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        int res = getResource();

        
        if (res == R.xml.preferences_search) {
            setHasOptionsMenu(true);
        }

        addPreferencesFromResource(res);

        PreferenceScreen screen = getPreferenceScreen();
        setPreferenceScreen(screen);
        mPrefsRequestId = ((GeckoPreferences)getActivity()).setupPreferences(screen);
    }

    




    private int getResource() {
        int resid = 0;

        String resourceName = getArguments().getString("resource");
        if (resourceName != null) {
            
            resid = getActivity().getResources().getIdentifier(resourceName,
                                                             "xml",
                                                             getActivity().getPackageName());
        }

        if (resid == 0) {
            
            Log.e(LOGTAG, "Failed to find resource: " + resourceName + ". Displaying default settings.");

            boolean isMultiPane = ((PreferenceActivity) getActivity()).onIsMultiPane();
            resid = isMultiPane ? R.xml.preferences_customize_tablet : R.xml.preferences;
        }

        return resid;
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        inflater.inflate(R.menu.preferences_search_menu, menu);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mPrefsRequestId > 0) {
            PrefsHelper.removeObserver(mPrefsRequestId);
        }
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        showOverflowMenu(activity);
    }

    





    private void showOverflowMenu(Activity activity) {
        try {
            ViewConfiguration config = ViewConfiguration.get(activity);
            Field menuOverflow = ViewConfiguration.class.getDeclaredField("sHasPermanentMenuKey");
            if (menuOverflow != null) {
                menuOverflow.setAccessible(true);
                menuOverflow.setBoolean(config, false);
            }
        } catch (Exception e) {
            Log.d(LOGTAG, "Failed to force overflow menu, ignoring.");
        }
    }
}
