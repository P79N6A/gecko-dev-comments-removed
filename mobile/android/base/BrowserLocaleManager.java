




package org.mozilla.gecko;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.Log;

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
















public class BrowserLocaleManager implements LocaleManager {
    private static final String LOG_TAG = "GeckoLocales";

    private static final String EVENT_LOCALE_CHANGED = "Locale:Changed";
    private static final String PREF_LOCALE = "locale";

    private static final String FALLBACK_LOCALE_TAG = "en-US";

    
    
    private volatile Locale currentLocale = null;

    private AtomicBoolean inited = new AtomicBoolean(false);
    private boolean systemLocaleDidChange = false;
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

    







    public static String getLanguageTag(final Locale locale) {
        
        

        String language = locale.getLanguage();  
        
        if (language.equals("iw")) {
            language = "he";
        } else if (language.equals("in")) {
            language = "id";
        } else if (language.equals("ji")) {
            language = "yi";
        }

        String country = locale.getCountry();    
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
                systemLocaleDidChange = true;
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
            return;
        }

        
        

        config.locale = current;

        
        
        
        

        
        
        
        Locale.setDefault(current);

        
        
        res.updateConfiguration(config, res.getDisplayMetrics());
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
    public void updateConfiguration(Context context, Locale locale) {
        Resources res = context.getResources();
        Configuration config = res.getConfiguration();

        
        
        config.locale = locale;
        res.updateConfiguration(config, res.getDisplayMetrics());
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
        settings.edit().putString(PREF_LOCALE, localeCode).commit();
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

        
        if (defaultLocale.equals(locale)) {
            return null;
        }

        Locale.setDefault(locale);
        currentLocale = locale;

        
        updateConfiguration(context, locale);

        return locale.toString();
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
