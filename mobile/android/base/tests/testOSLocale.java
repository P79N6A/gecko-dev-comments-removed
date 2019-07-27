



package org.mozilla.gecko.tests;

import java.util.Locale;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.PrefsHelper;

import android.content.SharedPreferences;


public class testOSLocale extends BaseTest {
    @Override
    public void setUp() throws Exception {
        super.setUp();

        
        
        
        
        final String profileName = getTestProfile().getName();
        mAsserter.info("Setup", "Clearing pref in " + profileName + ".");
        GeckoSharedPrefs.forProfileName(getActivity(), profileName)
                        .edit()
                        .remove("osLocale")
                        .apply();
    }

    public static class PrefState extends PrefsHelper.PrefHandlerBase {
        private static final String PREF_LOCALE_OS = "intl.locale.os";
        private static final String PREF_ACCEPT_LANG = "intl.accept_languages";

        private static final String[] TO_FETCH = {PREF_LOCALE_OS, PREF_ACCEPT_LANG};

        public volatile String osLocale;
        public volatile String acceptLanguages;

        private final Object waiter = new Object();

        public void fetch() throws InterruptedException {
            
            GeckoAppShell.sendEventToGeckoSync(GeckoEvent.createNoOpEvent());
            synchronized (waiter) {
                PrefsHelper.getPrefs(TO_FETCH, this);
                waiter.wait(MAX_WAIT_MS);
            }
        }

        @Override
        public void prefValue(String pref, String value) {
            switch (pref) {
            case PREF_LOCALE_OS:
                osLocale = value;
                return;
            case PREF_ACCEPT_LANG:
                acceptLanguages = value;
                return;
            }
        }

        @Override
        public void finish() {
            synchronized (waiter) {
                waiter.notify();
            }
        }
    }

    public void testOSLocale() throws Exception {
        blockForDelayedStartup();

        final SharedPreferences prefs = GeckoSharedPrefs.forProfile(getActivity());
        final PrefState state = new PrefState();

        state.fetch();

        
        
        
        
        
        
        
        
        
        
        final Locale fr = BrowserLocaleManager.parseLocaleCode("fr");
        BrowserLocaleManager.storeAndNotifyOSLocale(prefs, fr);

        state.fetch();

        mAsserter.is(state.osLocale, "fr", "We're in fr.");

        
        
        
        mAsserter.is(state.acceptLanguages, "en-us,fr,en", "We have the default en-US+fr Accept-Languages.");

        
        BrowserLocaleManager.getInstance().setSelectedLocale(getActivity(), "es-ES");

        state.fetch();

        mAsserter.is(state.osLocale, "fr", "We're still in fr.");

        
        
        
        final boolean isMultiLocaleBuild = false;

        
        final String SELECTED_LOCALES = "es-es,fr,";

        
        final String EXPECTED = SELECTED_LOCALES +
                                (isMultiLocaleBuild ? "es,en-us,en" :  
                                                      "en-us,en");     

        mAsserter.is(state.acceptLanguages, EXPECTED, "We have the right es-ES+fr Accept-Languages for this build.");

        
        final Locale en_US = BrowserLocaleManager.parseLocaleCode("en-US");
        BrowserLocaleManager.storeAndNotifyOSLocale(prefs, en_US);
        BrowserLocaleManager.getInstance().resetToSystemLocale(getActivity());

        state.fetch();

        mAsserter.is(state.osLocale, "en-US", "We're in en-US.");
        mAsserter.is(state.acceptLanguages, "en-us,en", "We have the default processed en-US Accept-Languages.");
    }
}