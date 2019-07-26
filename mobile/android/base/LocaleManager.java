




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
    private static final String PREF_LOCALE = "locale";

    
    
    private static volatile Locale currentLocale = null;

    private static volatile boolean inited = false;
    private static boolean systemLocaleDidChange = false;
    private static BroadcastReceiver receiver;

    






    public static void initialize(final Context context) {
        if (inited) {
            return;
        }

        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                systemLocaleDidChange = true;
            }
        };
        context.registerReceiver(receiver, new IntentFilter(Intent.ACTION_LOCALE_CHANGED));
        inited = true;
    }

    public static boolean systemLocaleDidChange() {
        return systemLocaleDidChange;
    }

    



    public static void correctLocale(Context context, Resources res, Configuration config) {
        final Locale current = getCurrentLocale(context);
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

    public static Locale getCurrentLocale(Context context) {
        if (currentLocale != null) {
            return currentLocale;
        }

        final String current = getPersistedLocale(context);
        if (current == null) {
            return null;
        }
        return currentLocale = parseLocaleCode(current);
    }

    






    private static String updateLocale(Context context, String localeCode) {
        
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

        
        Resources res = context.getResources();
        Configuration config = res.getConfiguration();
        config.locale = locale;
        res.updateConfiguration(config, res.getDisplayMetrics());

        return locale.toString();
    }

    public static void notifyGeckoOfLocaleChange(Locale locale) {
        
        GeckoEvent ev = GeckoEvent.createBroadcastEvent("Locale:Changed", getLanguageTag(locale));
        GeckoAppShell.sendEventToGecko(ev);
    }

    public static String getPersistedLocale(Context context) {
        final SharedPreferences settings = getSharedPreferences(context);
        final String locale = settings.getString(PREF_LOCALE, "");

        if ("".equals(locale)) {
            return null;
        }
        return locale;
    }

    private static void persistLocale(Context context, String localeCode) {
        final SharedPreferences settings = getSharedPreferences(context);
        settings.edit().putString(PREF_LOCALE, localeCode).commit();
    }

    private static SharedPreferences getSharedPreferences(Context context) {
        
        return GeckoSharedPrefs.forApp(context);
    }

    public static String getAndApplyPersistedLocale(Context context) {
        final long t1 = android.os.SystemClock.uptimeMillis();
        final String localeCode = getPersistedLocale(context);
        if (localeCode == null) {
            return null;
        }

        
        
        final String resultant = updateLocale(context, localeCode);

        final long t2 = android.os.SystemClock.uptimeMillis();
        Log.i(LOG_TAG, "Locale read and update took: " + (t2 - t1) + "ms.");
        return resultant;
    }

    




    public static String setSelectedLocale(Context context, String localeCode) {
        final String resultant = updateLocale(context, localeCode);

        
        
        
        
        
        persistLocale(context, localeCode);
        notifyGeckoOfLocaleChange(getCurrentLocale(context));
        return resultant;
    }
}

