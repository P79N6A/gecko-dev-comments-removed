




package org.mozilla.gecko.preferences;

import java.util.Locale;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.LocaleManager;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.R;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;





public class GeckoPreferenceFragment extends PreferenceFragment {

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Log.d(LOGTAG, "onConfigurationChanged: " + newConfig.locale);

        final Activity context = getActivity();

        final LocaleManager localeManager = BrowserLocaleManager.getInstance();
        final Locale changed = localeManager.onSystemConfigurationChanged(context, getResources(), newConfig, lastLocale);
        if (changed != null) {
            applyLocale(changed);
        }
    }

    private static final String LOGTAG = "GeckoPreferenceFragment";
    private int mPrefsRequestId;
    private Locale lastLocale = Locale.getDefault();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        
        getPreferenceManager().setSharedPreferencesName(GeckoSharedPrefs.APP_PREFS_NAME);

        int res = getResource();

        
        if (res == R.xml.preferences_search) {
            setHasOptionsMenu(true);
        }

        addPreferencesFromResource(res);

        PreferenceScreen screen = getPreferenceScreen();
        setPreferenceScreen(screen);
        mPrefsRequestId = ((GeckoPreferences)getActivity()).setupPreferences(screen);
    }

    








    private String getTitle() {
        final int res = getResource();
        if (res == R.xml.preferences_locale) {
            return getString(R.string.pref_category_language);
        }

        if (res == R.xml.preferences) {
            return getString(R.string.settings_title);
        }

        
        
        if (res == R.xml.preferences_vendor) {
            return getString(R.string.pref_category_vendor);
        }

        return null;
    }

    private void updateTitle() {
        final String newTitle = getTitle();
        if (newTitle == null) {
            Log.d(LOGTAG, "No new title to show.");
            return;
        }

        final PreferenceActivity activity = (PreferenceActivity) getActivity();
        if (Versions.feature11Plus && activity.isMultiPane()) {
            
            
            activity.showBreadCrumbs(newTitle, newTitle);
            return;
        }

        Log.v(LOGTAG, "Setting activity title to " + newTitle);
        activity.setTitle(newTitle);

        if (Versions.feature14Plus) {
            final ActionBar actionBar = activity.getActionBar();
            if (actionBar != null) {
                actionBar.setTitle(newTitle);
            }
        }
    }

    @Override
    public void onResume() {
        
        
        applyLocale(Locale.getDefault());
        super.onResume();
    }

    private void applyLocale(final Locale currentLocale) {
        final Context context = getActivity().getApplicationContext();

        BrowserLocaleManager.getInstance().updateConfiguration(context, currentLocale);

        if (!currentLocale.equals(lastLocale)) {
            
            Log.d(LOGTAG, "Locale changed: " + currentLocale);
            this.lastLocale = currentLocale;

            
            getPreferenceScreen().removeAll();
            addPreferencesFromResource(getResource());
        }

        
        updateTitle();
    }

    




    private int getResource() {
        int resid = 0;

        final String resourceName = getArguments().getString("resource");
        final Activity activity = getActivity();

        if (resourceName != null) {
            
            final Resources resources = activity.getResources();
            final String packageName = activity.getPackageName();
            resid = resources.getIdentifier(resourceName, "xml", packageName);
        }

        if (resid == 0) {
            
            Log.e(LOGTAG, "Failed to find resource: " + resourceName + ". Displaying default settings.");

            boolean isMultiPane = Versions.feature11Plus &&
                                  ((PreferenceActivity) activity).isMultiPane();
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
}