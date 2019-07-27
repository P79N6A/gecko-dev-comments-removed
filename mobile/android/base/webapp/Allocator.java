




package org.mozilla.gecko.webapp;

import java.util.ArrayList;

import org.mozilla.gecko.GeckoAppShell;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.util.Log;

public class Allocator {

    private final String LOGTAG = "GeckoWebappAllocator";

    private static final String PREFIX_ORIGIN = "webapp-origin-";
    private static final String PREFIX_PACKAGE_NAME = "webapp-package-name-";

    
    
    
    
    private static final String PREFIX_OLD_APP = "app";
    private static final String PREFIX_OLD_ICON = "icon";

    
    private final static int MAX_WEB_APPS = 100;

    protected static Allocator sInstance = null;
    public static Allocator getInstance() {
        return getInstance(GeckoAppShell.getContext());
    }

    public static synchronized Allocator getInstance(Context cx) {
        if (sInstance == null) {
            sInstance = new Allocator(cx);
        }

        return sInstance;
    }

    SharedPreferences mPrefs;

    protected Allocator(Context context) {
        mPrefs = context.getSharedPreferences("webapps", Context.MODE_PRIVATE | Context.MODE_MULTI_PROCESS);
    }

    private static String appKey(int index) {
        return PREFIX_PACKAGE_NAME + index;
    }

    public static String iconKey(int index) {
        return "web-app-color-" + index;
    }

    public static String originKey(int i) {
        return PREFIX_ORIGIN + i;
    }

    private static String oldAppKey(int index) {
        return PREFIX_OLD_APP + index;
    }

    private static String oldIconKey(int index) {
        return PREFIX_OLD_ICON + index;
    }

    public ArrayList<String> getInstalledPackageNames() {
        ArrayList<String> installedPackages = new ArrayList<String>();

        for (int i = 0; i < MAX_WEB_APPS; ++i) {
            if (mPrefs.contains(appKey(i))) {
                installedPackages.add(mPrefs.getString(appKey(i), ""));
            }
        }
        return installedPackages;
    }

    public synchronized int findOrAllocatePackage(final String packageName) {
        int index = getIndexForApp(packageName);
        if (index != -1)
            return index;

        for (int i = 0; i < MAX_WEB_APPS; ++i) {
            if (!mPrefs.contains(appKey(i))) {
                
                putPackageName(i, packageName);
                return i;
            }
        }

        
        return -1;
    }

    public synchronized void putPackageName(final int index, final String packageName) {
        mPrefs.edit().putString(appKey(index), packageName).apply();
    }

    public void updateColor(int index, int color) {
        mPrefs.edit().putInt(iconKey(index), color).apply();
    }

    public synchronized int getIndexForApp(String packageName) {
        return findSlotForPrefix(PREFIX_PACKAGE_NAME, packageName);
    }

    protected int findSlotForPrefix(String prefix, String value) {
        for (int i = 0; i < MAX_WEB_APPS; ++i) {
            if (mPrefs.getString(prefix + i, "").equals(value)) {
                return i;
            }
        }
        return -1;
    }

    public synchronized String getAppForIndex(int index) {
        return mPrefs.getString(appKey(index), null);
    }

    public synchronized int releaseIndexForApp(String app) {
        int index = getIndexForApp(app);
        if (index == -1)
            return -1;

        releaseIndex(index);
        return index;
    }

    public synchronized void releaseIndex(final int index) {
        mPrefs.edit().remove(appKey(index)).remove(iconKey(index)).remove(originKey(index)).apply();
    }

    public void putOrigin(int index, String origin) {
        mPrefs.edit().putString(originKey(index), origin).apply();
    }

    public String getOrigin(int index) {
        return mPrefs.getString(originKey(index), null);
    }

    public int getColor(int index) {
        return mPrefs.getInt(iconKey(index), -1);
    }

    



    public void maybeMigrateOldPrefs(int index) {
        if (!mPrefs.contains(oldAppKey(index))) {
            return;
        }

        Log.i(LOGTAG, "migrating old prefs");

        
        
        putOrigin(index, mPrefs.getString(oldAppKey(index), null));

        
        
        updateColor(index, mPrefs.getInt(oldIconKey(index), -1));

        
        mPrefs.edit().remove(oldAppKey(index)).remove(oldIconKey(index)).apply();
    }
}
