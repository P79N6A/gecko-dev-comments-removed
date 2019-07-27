


package org.mozilla.gecko;

import java.util.Arrays;
import java.util.Collections;
import java.util.EnumSet;
import java.util.HashSet;
import java.util.Set;

import org.mozilla.gecko.GeckoSharedPrefs.Flags;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.test.RenamingDelegatingContext;




public class TestGeckoSharedPrefs extends BrowserTestCase {

    private static class TestContext extends RenamingDelegatingContext {
        private static final String PREFIX = "TestGeckoSharedPrefs-";

        private final Set<String> usedPrefs;

        public TestContext(Context context) {
            super(context, PREFIX);
            usedPrefs = Collections.synchronizedSet(new HashSet<String>());
        }

        @Override
        public SharedPreferences getSharedPreferences(String name, int mode) {
            usedPrefs.add(name);
            return super.getSharedPreferences(PREFIX + name, mode);
        }

        public void clearUsedPrefs() {
            for (String prefsName : usedPrefs) {
                getSharedPreferences(prefsName, 0).edit().clear().commit();
            }

            usedPrefs.clear();
        }
    }

    private static final EnumSet<Flags> disableMigrations = EnumSet.of(Flags.DISABLE_MIGRATIONS);

    private TestContext context;

    protected void setUp() {
        context = new TestContext(getApplicationContext());
    }

    protected void tearDown() {
        context.clearUsedPrefs();
        GeckoSharedPrefs.reset();
    }

    public void testDisableMigrations() {
        
        assertEquals(0, GeckoSharedPrefs.getVersion(context));

        
        GeckoSharedPrefs.forApp(context, disableMigrations);
        GeckoSharedPrefs.forProfile(context, disableMigrations);
        GeckoSharedPrefs.forProfileName(context, "someProfile", disableMigrations);

        
        assertEquals(0, GeckoSharedPrefs.getVersion(context));
    }

    public void testPrefsVersion() {
        
        assertEquals(0, GeckoSharedPrefs.getVersion(context));

        
        GeckoSharedPrefs.forApp(context);

        
        assertEquals(GeckoSharedPrefs.PREFS_VERSION, GeckoSharedPrefs.getVersion(context));
    }

    public void testMigrateFromPreferenceManager() {
        SharedPreferences appPrefs = GeckoSharedPrefs.forApp(context, disableMigrations);
        assertTrue(appPrefs.getAll().isEmpty());
        final Editor appEditor = appPrefs.edit();

        SharedPreferences profilePrefs = GeckoSharedPrefs.forProfileName(context, GeckoProfile.DEFAULT_PROFILE, disableMigrations);
        assertTrue(profilePrefs.getAll().isEmpty());
        final Editor profileEditor = profilePrefs.edit();

        final SharedPreferences pmPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        assertTrue(pmPrefs.getAll().isEmpty());
        Editor pmEditor = pmPrefs.edit();

        
        
        pmEditor.putInt("int_key", 23);
        pmEditor.putLong("long_key", 23L);
        pmEditor.putString("string_key", "23");
        pmEditor.putFloat("float_key", 23.3f);

        final String[] profileKeys = {
            "string_profile",
            "int_profile"
        };

        
        
        pmEditor.putString(profileKeys[0], "24");
        pmEditor.putInt(profileKeys[1], 24);

        
        pmEditor.commit();
        assertEquals(6, pmPrefs.getAll().size());

        
        pmEditor = GeckoSharedPrefs.migrateFromPreferenceManager(context, appEditor,
                profileEditor, Arrays.asList(profileKeys));

        
        appEditor.commit();
        profileEditor.commit();
        pmEditor.commit();

        
        assertTrue(pmPrefs.getAll().isEmpty());

        
        assertEquals(4, appPrefs.getAll().size());

        
        for (int i = 0; i < profileKeys.length; i++) {
            assertFalse(appPrefs.contains(profileKeys[i]));
        }

        
        assertEquals(23, appPrefs.getInt("int_key", 0));
        assertEquals(23L, appPrefs.getLong("long_key", 0L));
        assertEquals("23", appPrefs.getString("string_key", ""));
        assertEquals(23.3f, appPrefs.getFloat("float_key", 0));

        assertEquals(2, profilePrefs.getAll().size());
        assertEquals("24", profilePrefs.getString(profileKeys[0], ""));
        assertEquals(24, profilePrefs.getInt(profileKeys[1], 0));
    }
}
