




package org.mozilla.gecko.updater;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.GeckoJarReader;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.util.Log;

import java.net.URL;

public class UpdateServiceHelper {
    public static final String ACTION_REGISTER_FOR_UPDATES = AppConstants.ANDROID_PACKAGE_NAME + ".REGISTER_FOR_UPDATES";
    public static final String ACTION_UNREGISTER_FOR_UPDATES = AppConstants.ANDROID_PACKAGE_NAME + ".UNREGISTER_FOR_UPDATES";
    public static final String ACTION_CHECK_FOR_UPDATE = AppConstants.ANDROID_PACKAGE_NAME + ".CHECK_FOR_UPDATE";
    public static final String ACTION_CHECK_UPDATE_RESULT = AppConstants.ANDROID_PACKAGE_NAME + ".CHECK_UPDATE_RESULT";
    public static final String ACTION_DOWNLOAD_UPDATE = AppConstants.ANDROID_PACKAGE_NAME + ".DOWNLOAD_UPDATE";
    public static final String ACTION_APPLY_UPDATE = AppConstants.ANDROID_PACKAGE_NAME + ".APPLY_UPDATE";
    public static final String ACTION_CANCEL_DOWNLOAD = AppConstants.ANDROID_PACKAGE_NAME + ".CANCEL_DOWNLOAD";

    
    public static final int FLAG_FORCE_DOWNLOAD = 1;
    public static final int FLAG_OVERWRITE_EXISTING = 1 << 1;
    public static final int FLAG_REINSTALL = 1 << 2;
    public static final int FLAG_RETRY = 1 << 3;

    
    public static final String EXTRA_AUTODOWNLOAD_NAME = "autodownload";

    
    public static final int AUTODOWNLOAD_WIFI = 0;
    public static final int AUTODOWNLOAD_DISABLED = 1;
    public static final int AUTODOWNLOAD_ENABLED = 2;

    
    public static final String EXTRA_UPDATE_FLAGS_NAME = "updateFlags";

    
    public static final String EXTRA_PACKAGE_PATH_NAME = "packagePath";

    private static final String LOGTAG = "UpdateServiceHelper";
    private static final String DEFAULT_UPDATE_LOCALE = "en-US";

    private static final String UPDATE_URL;

    
    private static volatile boolean isEnabled = true;

    static {
        final String pkgSpecial;
        if (AppConstants.MOZ_PKG_SPECIAL != null) {
            pkgSpecial = "-" + AppConstants.MOZ_PKG_SPECIAL;
        } else {
            pkgSpecial = "";
        }
        UPDATE_URL = "https://aus4.mozilla.org/update/4/" + AppConstants.MOZ_APP_BASENAME + "/" +
                     AppConstants.MOZ_APP_VERSION         +
                     "/%BUILDID%/Android_"                + AppConstants.MOZ_APP_ABI + pkgSpecial +
                     "/%LOCALE%/"                         + AppConstants.MOZ_UPDATE_CHANNEL +
                     "/%OS_VERSION%/default/default/"     + AppConstants.MOZILLA_VERSION +
                     "/update.xml";
    }

    public enum CheckUpdateResult {
        
        NOT_AVAILABLE,
        AVAILABLE,
        DOWNLOADING,
        DOWNLOADED
    }

    @RobocopTarget
    public static void setEnabled(final boolean enabled) {
        isEnabled = enabled;
    }

    public static URL getUpdateUrl(Context context, boolean force) {
        PackageManager pm = context.getPackageManager();

        String locale = null;
        try {
            ApplicationInfo info = pm.getApplicationInfo(AppConstants.ANDROID_PACKAGE_NAME, 0);
            String updateLocaleUrl = "jar:jar:file://" + info.sourceDir + "!/" + AppConstants.OMNIJAR_NAME + "!/update.locale";

            locale = GeckoJarReader.getText(updateLocaleUrl);

            if (locale != null)
                locale = locale.trim();
            else
                locale = DEFAULT_UPDATE_LOCALE;
        } catch (android.content.pm.PackageManager.NameNotFoundException e) {
            
            Log.i(LOGTAG, "Failed to read update locale file, falling back to " + DEFAULT_UPDATE_LOCALE);
            locale = DEFAULT_UPDATE_LOCALE;
        }

        String url = UPDATE_URL.replace("%LOCALE%", locale).
            replace("%OS_VERSION%", Build.VERSION.RELEASE).
            replace("%BUILDID%", force ? "0" : AppConstants.MOZ_APP_BUILDID);

        try {
            return new URL(url);
        } catch (java.net.MalformedURLException e) {
            Log.e(LOGTAG, "Failed to create update url: ", e);
            return null;
        }
    }

    public static boolean isUpdaterEnabled() {
        return AppConstants.MOZ_UPDATER && isEnabled;
    }

    public static void registerForUpdates(Context context, String policy) {
        if (policy == null)
            return;

        int intPolicy;
        if (policy.equals("wifi")) {
            intPolicy = AUTODOWNLOAD_WIFI;
        } else if (policy.equals("disabled")) {
            intPolicy = AUTODOWNLOAD_DISABLED;
        } else if (policy.equals("enabled")) {
            intPolicy = AUTODOWNLOAD_ENABLED;
        } else {
            Log.w(LOGTAG, "Unhandled autoupdate policy: " + policy);
            return;
        }

        registerForUpdates(context, intPolicy);
    }

    
    public static void registerForUpdates(Context context, int policy) {
        if (!isUpdaterEnabled())
            return;

        Intent intent = new Intent(UpdateServiceHelper.ACTION_REGISTER_FOR_UPDATES, null, context, UpdateService.class);
        intent.putExtra(EXTRA_AUTODOWNLOAD_NAME, policy);

        context.startService(intent);
    }
}
