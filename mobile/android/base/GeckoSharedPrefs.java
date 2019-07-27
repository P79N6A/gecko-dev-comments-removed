



package org.mozilla.gecko;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;

import org.mozilla.gecko.mozglue.RobocopTarget;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.StrictMode;
import android.preference.PreferenceManager;
import android.util.Log;




















@RobocopTarget
public final class GeckoSharedPrefs {
    private static final String LOGTAG = "GeckoSharedPrefs";

    
    public static final int PREFS_VERSION = 1;

    
    public static final String APP_PREFS_NAME = "GeckoApp";

    
    public static final String PROFILE_PREFS_NAME_PREFIX = "GeckoProfile-";

    
    private static final String PREFS_VERSION_KEY = "gecko_shared_prefs_migration";

    
    private static final EnumSet<Flags> disableMigrations = EnumSet.of(Flags.DISABLE_MIGRATIONS);

    
    
    private static final String[] PROFILE_MIGRATIONS_0_TO_1 = {
        "home_panels",
        "home_locale"
    };

    
    private static volatile boolean migrationDone;

    public enum Flags {
        DISABLE_MIGRATIONS
    }

    public static SharedPreferences forApp(Context context) {
        return forApp(context, EnumSet.noneOf(Flags.class));
    }

    



    public static SharedPreferences forApp(Context context, EnumSet<Flags> flags) {
        if (flags != null && !flags.contains(Flags.DISABLE_MIGRATIONS)) {
            migrateIfNecessary(context);
        }

        return context.getSharedPreferences(APP_PREFS_NAME, 0);
    }

    public static SharedPreferences forProfile(Context context) {
        return forProfile(context, EnumSet.noneOf(Flags.class));
    }

    




    public static SharedPreferences forProfile(Context context, EnumSet<Flags> flags) {
        String profileName = GeckoProfile.get(context).getName();
        if (profileName == null) {
            throw new IllegalStateException("Could not get current profile name");
        }

        return forProfileName(context, profileName, flags);
    }

    public static SharedPreferences forProfileName(Context context, String profileName) {
        return forProfileName(context, profileName, EnumSet.noneOf(Flags.class));
    }

    



    public static SharedPreferences forProfileName(Context context, String profileName,
            EnumSet<Flags> flags) {
        if (flags != null && !flags.contains(Flags.DISABLE_MIGRATIONS)) {
            migrateIfNecessary(context);
        }

        final String prefsName = PROFILE_PREFS_NAME_PREFIX + profileName;
        return context.getSharedPreferences(prefsName, 0);
    }

    


    public static int getVersion(Context context) {
        return forApp(context, disableMigrations).getInt(PREFS_VERSION_KEY, 0);
    }

    


    public static synchronized void reset() {
        migrationDone = false;
    }

    




    private static synchronized void migrateIfNecessary(final Context context) {
        if (migrationDone) {
            return;
        }

        
        
        
        
        final StrictMode.ThreadPolicy savedPolicy = StrictMode.allowThreadDiskReads();
        StrictMode.allowThreadDiskWrites();
        try {
            performMigration(context);
        } finally {
            StrictMode.setThreadPolicy(savedPolicy);
        }

        migrationDone = true;
    }

    private static void performMigration(Context context) {
        final SharedPreferences appPrefs = forApp(context, disableMigrations);

        final int currentVersion = appPrefs.getInt(PREFS_VERSION_KEY, 0);
        Log.d(LOGTAG, "Current version = " + currentVersion + ", prefs version = " + PREFS_VERSION);

        if (currentVersion == PREFS_VERSION) {
            return;
        }

        Log.d(LOGTAG, "Performing migration");

        final Editor appEditor = appPrefs.edit();

        
        
        
        final String defaultProfileName;
        try {
            defaultProfileName = GeckoProfile.getDefaultProfileName(context);
        } catch (Exception e) {
            throw new IllegalStateException("Failed to get default profile name for migration");
        }

        final Editor profileEditor = forProfileName(context, defaultProfileName, disableMigrations).edit();

        List<String> profileKeys;
        Editor pmEditor = null;

        for (int v = currentVersion + 1; v <= PREFS_VERSION; v++) {
            Log.d(LOGTAG, "Migrating to version = " + v);

            switch (v) {
                case 1:
                    profileKeys = Arrays.asList(PROFILE_MIGRATIONS_0_TO_1);
                    pmEditor = migrateFromPreferenceManager(context, appEditor, profileEditor, profileKeys);
                    break;
            }
        }

        
        appEditor.putInt(PREFS_VERSION_KEY, PREFS_VERSION);

        appEditor.apply();
        profileEditor.apply();
        if (pmEditor != null) {
            pmEditor.apply();
        }

        Log.d(LOGTAG, "All keys have been migrated");
    }

    




    public static Editor migrateFromPreferenceManager(Context context, Editor appEditor,
            Editor profileEditor, List<String> profileKeys) {
        Log.d(LOGTAG, "Migrating from PreferenceManager");

        final SharedPreferences pmPrefs =
                PreferenceManager.getDefaultSharedPreferences(context);

        for (Map.Entry<String, ?> entry : pmPrefs.getAll().entrySet()) {
            final String key = entry.getKey();

            final Editor to;
            if (profileKeys.contains(key)) {
                to = profileEditor;
            } else {
                to = appEditor;
            }

            putEntry(to, key, entry.getValue());
        }

        
        
        return pmPrefs.edit().clear();
    }

    private static void putEntry(Editor to, String key, Object value) {
        Log.d(LOGTAG, "Migrating key = " + key + " with value = " + value);

        if (value instanceof String) {
            to.putString(key, (String) value);
        } else if (value instanceof Boolean) {
            to.putBoolean(key, (Boolean) value);
        } else if (value instanceof Long) {
            to.putLong(key, (Long) value);
        } else if (value instanceof Float) {
            to.putFloat(key, (Float) value);
        } else if (value instanceof Integer) {
            to.putInt(key, (Integer) value);
        } else {
            throw new IllegalStateException("Unrecognized value type for key: " + key);
        }
    }
}