




package org.mozilla.gecko;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.Log;

import java.util.Locale;
















public class LocaleManager {
    private static final String LOG_TAG = "GeckoLocales";

    
    
    private static volatile ContextGetter getter = null;
    private static volatile Locale currentLocale = null;

    private static volatile boolean inited = false;
    private static boolean systemLocaleDidChange = false;
    private static BroadcastReceiver receiver;

    public static void setContextGetter(ContextGetter getter) {
        Log.d(LOG_TAG, "Calling setContextGetter: " + getter);
        LocaleManager.getter = getter;
    }

    public static void initialize() {
        if (inited) {
            return;
        }

        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                systemLocaleDidChange = true;
            }
        };
        getContext().registerReceiver(receiver, new IntentFilter(Intent.ACTION_LOCALE_CHANGED));
        inited = true;
    }

    public static boolean systemLocaleDidChange() {
        return systemLocaleDidChange;
    }

    private static Context getContext() {
        if (getter == null) {
            throw new IllegalStateException("No ContextGetter; cannot fetch context.");
        }
        return getter.getContext();
    }

    private static SharedPreferences getSharedPreferences() {
        if (getter == null) {
            throw new IllegalStateException("No ContextGetter; cannot fetch prefs.", new RuntimeException("No prefs."));
        }
        return getter.getSharedPreferences();
    }

    



    public static void correctLocale(Resources res, Configuration config) {
        Locale current = getCurrentLocale();
        if (current == null) {
            return;
        }

        
        

        config.locale = current;

        
        
        
        

        
        
        
        Locale.setDefault(current);

        
        
        res.updateConfiguration(config, res.getDisplayMetrics());
    }

    private static Locale parseLocaleCode(final String localeCode) {
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

    public static Locale getCurrentLocale() {
        if (currentLocale != null) {
            return currentLocale;
        }

        final String current = getPersistedLocale();
        if (current == null) {
            return null;
        }
        return currentLocale = parseLocaleCode(current);
    }

    






    private static String updateLocale(String localeCode) {
        
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

        
        Resources res = getContext().getResources();
        Configuration config = res.getConfiguration();
        config.locale = locale;
        res.updateConfiguration(config, res.getDisplayMetrics());

        return locale.toString();
    }

    public static void notifyGeckoOfLocaleChange(Locale locale) {
        
        GeckoEvent ev = GeckoEvent.createBroadcastEvent("Locale:Changed", getLanguageTag(locale));
        GeckoAppShell.sendEventToGecko(ev);
    }

    private static String getPrefName() {
        return getContext().getPackageName() + ".locale";
    }

    public static String getPersistedLocale() {
        final SharedPreferences settings = getSharedPreferences();

        
        
        
        final String locale = settings.getString(getPrefName(), "");

        if ("".equals(locale)) {
            return null;
        }
        return locale;
    }

    private static void persistLocale(String localeCode) {
        final SharedPreferences settings = getSharedPreferences();
        settings.edit().putString(getPrefName(), localeCode).commit();
    }

    public static String getAndApplyPersistedLocale() {
        final long t1 = android.os.SystemClock.uptimeMillis();
        final String localeCode = getPersistedLocale();
        if (localeCode == null) {
            return null;
        }

        
        
        updateLocale(localeCode);

        final long t2 = android.os.SystemClock.uptimeMillis();
        Log.i(LOG_TAG, "Locale read and update took: " + (t2 - t1) + "ms.");
        return localeCode;
    }

    




    public static String setSelectedLocale(String localeCode) {
        final String resultant = updateLocale(localeCode);

        
        
        
        
        
        persistLocale(localeCode);
        notifyGeckoOfLocaleChange(getCurrentLocale());
        return resultant;
    }
}

