




package org.mozilla.gecko;

import android.content.Context;
import android.content.SharedPreferences;

public class WebAppAllocator {
    
    private final static int MAX_WEB_APPS = 100;

    protected static GeckoApp sContext = null;
    protected static WebAppAllocator sInstance = null;
    public static WebAppAllocator getInstance() {
        return getInstance(GeckoApp.mAppContext);
    }

    public static synchronized WebAppAllocator getInstance(Context cx) {
        if (sInstance == null) {
            if (!(cx instanceof GeckoApp))
                throw new RuntimeException("Context needs to be a GeckoApp");
                
            sContext = (GeckoApp) cx;
            sInstance = new WebAppAllocator(cx);
        }

        if (cx != sContext)
            throw new RuntimeException("Tried to get WebAppAllocator instance for different context than it was created for");

        return sInstance;
    }

    SharedPreferences mPrefs;

    protected WebAppAllocator(Context context) {
        mPrefs = context.getSharedPreferences("webapps", Context.MODE_PRIVATE | Context.MODE_MULTI_PROCESS);
    }

    static String appKey(int index) {
        return "app" + index;
    }

    public synchronized int findAndAllocateIndex(String app) {
        int index = getIndexForApp(app);
        if (index != -1)
            return index;

        for (int i = 0; i < MAX_WEB_APPS; ++i) {
            if (!mPrefs.contains(appKey(i))) {
                
                mPrefs.edit()
                    .putString(appKey(i), app)
                    .apply();
                return i;
            }
        }

        
        return -1;
    }

    public synchronized int getIndexForApp(String app) {
        for (int i = 0; i < MAX_WEB_APPS; ++i) {
            if (mPrefs.getString(appKey(i), "").equals(app)) {
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

    public synchronized void releaseIndex(int index) {
        mPrefs.edit()
            .remove(appKey(index))
            .apply();
    }
}
