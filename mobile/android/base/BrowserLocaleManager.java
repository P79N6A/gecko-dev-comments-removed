




package org.mozilla.gecko;

import java.io.File;
import java.util.Collection;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.util.GeckoJarReader;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.Log;
















public class BrowserLocaleManager implements LocaleManager {
    private static final String LOG_TAG = "GeckoLocales";

    private static final String EVENT_LOCALE_CHANGED = "Locale:Changed";
    private static final String PREF_LOCALE = "locale";

    private static final String FALLBACK_LOCALE_TAG = "en-US";

    
    
    private volatile Locale currentLocale;
    private volatile Locale systemLocale = Locale.getDefault();

    private AtomicBoolean inited = new AtomicBoolean(false);
    private boolean systemLocaleDidChange;
    private BroadcastReceiver receiver;

    private static AtomicReference<LocaleManager> instance = new AtomicReference<LocaleManager>();

    public static LocaleManager getInstance() {
        LocaleManager localeManager = instance.get();
        if (localeManager != null) {
            return localeManager;
        }

        localeManager = new BrowserLocaleManager();
        if (instance.compareAndSet(null, localeManager)) {
            return localeManager;
        } else {
            return instance.get();
        }
    }

    public boolean isEnabled() {
        return AppConstants.MOZ_LOCALE_SWITCHER;
    }

    







    public static String getLanguage(final Locale locale) {
        final String language = locale.getLanguage();  
        
        if (language.equals("iw")) {
            return "he";
        }

        if (language.equals("in")) {
            return "id";
        }

        if (language.equals("ji")) {
            return "yi";
        }

        return language;
    }

    







    public static String getLanguageTag(final Locale locale) {
        
        

        final String language = getLanguage(locale);
        final String country = locale.getCountry();    
        if (country.equals("")) {
            return language;
        }
        return language + "-" + country;
    }

    public static Locale parseLocaleCode(final String localeCode) {
        int index;
        if ((index = localeCode.indexOf('-')) != -1 ||
            (index = localeCode.indexOf('_')) != -1) {
            final String langCode = localeCode.substring(0, index);
            final String countryCode = localeCode.substring(index + 1);
            return new Locale(langCode, countryCode);
        } else {
            return new Locale(localeCode);
        }
    }

    






    @Override
    public void initialize(final Context context) {
        if (!inited.compareAndSet(false, true)) {
            return;
        }

        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final Locale current = systemLocale;

                
                
                
                
                
                systemLocale = context.getResources().getConfiguration().locale;
                systemLocaleDidChange = true;

                Log.d(LOG_TAG, "System locale changed from " + current + " to " + systemLocale);
            }
        };
        context.registerReceiver(receiver, new IntentFilter(Intent.ACTION_LOCALE_CHANGED));
    }

    @Override
    public boolean systemLocaleDidChange() {
        return systemLocaleDidChange;
    }

    



    @Override
    public void correctLocale(Context context, Resources res, Configuration config) {
        final Locale current = getCurrentLocale(context);
        if (current == null) {
            Log.d(LOG_TAG, "No selected locale. No correction needed.");
            return;
        }

        
        

        config.locale = current;

        
        
        
        

        
        
        
        Locale.setDefault(current);

        
        
        res.updateConfiguration(config, null);
    }

    



























    @Override
    public Locale onSystemConfigurationChanged(final Context context, final Resources resources, final Configuration configuration, final Locale currentActivityLocale) {
        if (!isMirroringSystemLocale(context)) {
            correctLocale(context, resources, configuration);
        }

        final Locale changed = configuration.locale;
        if (changed.equals(currentActivityLocale)) {
            return null;
        }

        return changed;
    }

    @Override
    public String getAndApplyPersistedLocale(Context context) {
        initialize(context);

        final long t1 = android.os.SystemClock.uptimeMillis();
        final String localeCode = getPersistedLocale(context);
        if (localeCode == null) {
            return null;
        }

        
        
        final String resultant = updateLocale(context, localeCode);

        if (resultant == null) {
            
            updateConfiguration(context, currentLocale);
        }

        final long t2 = android.os.SystemClock.uptimeMillis();
        Log.i(LOG_TAG, "Locale read and update took: " + (t2 - t1) + "ms.");
        return resultant;
    }

    




    @Override
    public String setSelectedLocale(Context context, String localeCode) {
        final String resultant = updateLocale(context, localeCode);

        
        
        
        
        
        persistLocale(context, localeCode);

        
        GeckoEvent ev = GeckoEvent.createBroadcastEvent(EVENT_LOCALE_CHANGED, BrowserLocaleManager.getLanguageTag(getCurrentLocale(context)));
        GeckoAppShell.sendEventToGecko(ev);

        return resultant;
    }

    @Override
    public void resetToSystemLocale(Context context) {
        
        final SharedPreferences settings = getSharedPreferences(context);
        settings.edit().remove(PREF_LOCALE).apply();

        
        updateLocale(context, systemLocale);

        
        GeckoEvent ev = GeckoEvent.createBroadcastEvent(EVENT_LOCALE_CHANGED, "");
        GeckoAppShell.sendEventToGecko(ev);
    }

    




    @Override
    public void updateConfiguration(Context context, Locale locale) {
        Resources res = context.getResources();
        Configuration config = res.getConfiguration();

        
        
        config.locale = locale;
        res.updateConfiguration(config, null);
    }

    private SharedPreferences getSharedPreferences(Context context) {
        return GeckoSharedPrefs.forApp(context);
    }

    private String getPersistedLocale(Context context) {
        final SharedPreferences settings = getSharedPreferences(context);
        final String locale = settings.getString(PREF_LOCALE, "");

        if ("".equals(locale)) {
            return null;
        }
        return locale;
    }

    private void persistLocale(Context context, String localeCode) {
        final SharedPreferences settings = getSharedPreferences(context);
        settings.edit().putString(PREF_LOCALE, localeCode).apply();
    }

    @Override
    public Locale getCurrentLocale(Context context) {
        if (currentLocale != null) {
            return currentLocale;
        }

        final String current = getPersistedLocale(context);
        if (current == null) {
            return null;
        }
        return currentLocale = parseLocaleCode(current);
    }

    






    private String updateLocale(Context context, String localeCode) {
        
        final Locale defaultLocale = Locale.getDefault();
        if (defaultLocale.toString().equals(localeCode)) {
            return null;
        }

        final Locale locale = parseLocaleCode(localeCode);

        return updateLocale(context, locale);
    }

    private String updateLocale(Context context, final Locale locale) {
        
        if (Locale.getDefault().equals(locale)) {
            return null;
        }

        Locale.setDefault(locale);
        currentLocale = locale;

        
        updateConfiguration(context, locale);

        return locale.toString();
    }

    private boolean isMirroringSystemLocale(final Context context) {
        return getPersistedLocale(context) == null;
    }

    

















    public static Collection<String> getPackagedLocaleTags(final Context context) {
        final String resPath = "res/multilocale.json";
        final String apkPath = context.getPackageResourcePath();

        final String jarURL = "jar:jar:" + new File(apkPath).toURI() + "!/" +
                              AppConstants.OMNIJAR_NAME + "!/" +
                              resPath;

        final String contents = GeckoJarReader.getText(jarURL);
        if (contents == null) {
            
            return null;
        }

        try {
            final JSONObject multilocale = new JSONObject(contents);
            final JSONArray locales = multilocale.getJSONArray("locales");
            if (locales == null) {
                Log.e(LOG_TAG, "No 'locales' array in multilocales.json!");
                return null;
            }

            final Set<String> out = new HashSet<String>(locales.length());
            for (int i = 0; i < locales.length(); ++i) {
                
                
                
                out.add(locales.getString(i));
            }

            return out;
        } catch (JSONException e) {
            Log.e(LOG_TAG, "Unable to parse multilocale.json.", e);
            return null;
        }
    }

    



    public static String getFallbackLocaleTag() {
        return FALLBACK_LOCALE_TAG;
    }
}
