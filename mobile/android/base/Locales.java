



package org.mozilla.gecko;

import java.util.Locale;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.LocaleManager;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.StrictMode;
import android.support.v4.app.FragmentActivity;










public class Locales {
    public static void initializeLocale(Context context) {
        final LocaleManager localeManager = BrowserLocaleManager.getInstance();
        final StrictMode.ThreadPolicy savedPolicy = StrictMode.allowThreadDiskReads();
        StrictMode.allowThreadDiskWrites();
        try {
            localeManager.getAndApplyPersistedLocale(context);
        } finally {
            StrictMode.setThreadPolicy(savedPolicy);
        }
    }

    public static class LocaleAwareFragmentActivity extends FragmentActivity {
        @Override
        protected void onCreate(Bundle savedInstanceState) {
            Locales.initializeLocale(getApplicationContext());
            super.onCreate(savedInstanceState);
        }
    }

    public static class LocaleAwareActivity extends Activity {
        @Override
        protected void onCreate(Bundle savedInstanceState) {
            Locales.initializeLocale(getApplicationContext());
            super.onCreate(savedInstanceState);
        }
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
        }

        return new Locale(localeCode);
    }
}
