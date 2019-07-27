




package org.mozilla.gecko;

import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.util.Log;
import android.view.Surface;
import android.app.Activity;

import java.util.Arrays;
import java.util.List;







public class GeckoScreenOrientation {
    private static final String LOGTAG = "GeckoScreenOrientation";

    
    public enum ScreenOrientation {
        NONE(0),
        PORTRAIT_PRIMARY(1 << 0),
        PORTRAIT_SECONDARY(1 << 1),
        LANDSCAPE_PRIMARY(1 << 2),
        LANDSCAPE_SECONDARY(1 << 3),
        DEFAULT(1 << 4);

        public final short value;

        private ScreenOrientation(int value) {
            this.value = (short)value;
        }

        public static ScreenOrientation get(short value) {
            switch (value) {
                case (1 << 0): return PORTRAIT_PRIMARY;
                case (1 << 1): return PORTRAIT_SECONDARY;
                case (1 << 2): return LANDSCAPE_PRIMARY;
                case (1 << 3): return LANDSCAPE_SECONDARY;
                case (1 << 4): return DEFAULT;
                default: return NONE;
            }
        }
    }

    
    private static GeckoScreenOrientation sInstance;
    
    private static final ScreenOrientation DEFAULT_SCREEN_ORIENTATION = ScreenOrientation.DEFAULT;
    
    private static final int DEFAULT_ROTATION = Surface.ROTATION_0;
    
    private ScreenOrientation mDefaultScreenOrientation;
    
    private ScreenOrientation mScreenOrientation;
    
    private boolean mShouldNotify = true;
    
    private static final String DEFAULT_SCREEN_ORIENTATION_PREF = "app.orientation.default";

    public GeckoScreenOrientation() {
        PrefsHelper.getPref(DEFAULT_SCREEN_ORIENTATION_PREF, new PrefsHelper.PrefHandlerBase() {
            @Override public void prefValue(String pref, String value) {
                
                mDefaultScreenOrientation = screenOrientationFromArrayString(value);
                setRequestedOrientation(mDefaultScreenOrientation);
            }
        });

        mDefaultScreenOrientation = DEFAULT_SCREEN_ORIENTATION;
        update();
    }

    public static GeckoScreenOrientation getInstance() {
        if (sInstance == null) {
            sInstance = new GeckoScreenOrientation();
        }
        return sInstance;
    }

    


    public void enableNotifications() {
        update();
        mShouldNotify = true;
    }

    


    public void disableNotifications() {
        mShouldNotify = false;
    }

    





    public boolean update() {
        Activity activity = GeckoAppShell.getGeckoInterface().getActivity();
        if (activity == null) {
            return false;
        }
        Configuration config = activity.getResources().getConfiguration();
        return update(config.orientation);
    }

    








    public boolean update(int aAndroidOrientation) {
        return update(getScreenOrientation(aAndroidOrientation, getRotation()));
    }

    







    public boolean update(ScreenOrientation aScreenOrientation) {
        if (mScreenOrientation == aScreenOrientation) {
            return false;
        }
        mScreenOrientation = aScreenOrientation;
        Log.d(LOGTAG, "updating to new orientation " + mScreenOrientation);
        if (mShouldNotify) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createScreenOrientationEvent(mScreenOrientation.value));
        }
        return true;
    }

    


    public int getAndroidOrientation() {
        return screenOrientationToAndroidOrientation(getScreenOrientation());
    }

    



    public ScreenOrientation getScreenOrientation() {
        return mScreenOrientation;
    }

    






    public void lock(int aAndroidOrientation) {
        lock(getScreenOrientation(aAndroidOrientation, getRotation()));
    }

    









    public boolean lock(ScreenOrientation aScreenOrientation) {
        Log.d(LOGTAG, "locking to " + aScreenOrientation);
        update(aScreenOrientation);
        return setRequestedOrientation(aScreenOrientation);
    }

    




    public boolean unlock() {
        Log.d(LOGTAG, "unlocking");
        setRequestedOrientation(mDefaultScreenOrientation);
        return update();
    }

    










    private boolean setRequestedOrientation(ScreenOrientation aScreenOrientation) {
        int activityOrientation = screenOrientationToActivityInfoOrientation(aScreenOrientation);
        Activity activity = GeckoAppShell.getGeckoInterface().getActivity();
        if (activity == null) {
            Log.w(LOGTAG, "setRequestOrientation: failed to get activity");
        }
        if (activity.getRequestedOrientation() == activityOrientation) {
            return false;
        }
        activity.setRequestedOrientation(activityOrientation);
        return true;
    }

    









    private ScreenOrientation getScreenOrientation(int aAndroidOrientation, int aRotation) {
        boolean isPrimary = aRotation == Surface.ROTATION_0 || aRotation == Surface.ROTATION_90;
        if (aAndroidOrientation == Configuration.ORIENTATION_PORTRAIT) {
            if (isPrimary) {
                
                
                return ScreenOrientation.PORTRAIT_PRIMARY;
            }
            return ScreenOrientation.PORTRAIT_SECONDARY;
        }
        if (aAndroidOrientation == Configuration.ORIENTATION_LANDSCAPE) {
            if (isPrimary) {
                
                
                return ScreenOrientation.LANDSCAPE_PRIMARY;
            }
            return ScreenOrientation.LANDSCAPE_SECONDARY;
        }
        return ScreenOrientation.NONE;
    }

    


    private int getRotation() {
        Activity activity = GeckoAppShell.getGeckoInterface().getActivity();
        if (activity == null) {
            Log.w(LOGTAG, "getRotation: failed to get activity");
            return DEFAULT_ROTATION;
        }
        return activity.getWindowManager().getDefaultDisplay().getRotation();
    }

    







    public static ScreenOrientation screenOrientationFromArrayString(String aArray) {
        List<String> orientations = Arrays.asList(aArray.split(","));
        if (orientations.size() == 0) {
            
            Log.w(LOGTAG, "screenOrientationFromArrayString: no orientation in string");
            return DEFAULT_SCREEN_ORIENTATION;
        }

        
        
        return screenOrientationFromString(orientations.get(0));
    }

    







    public static ScreenOrientation screenOrientationFromString(String aStr) {
        if ("portrait".equals(aStr)) {
            return ScreenOrientation.PORTRAIT_PRIMARY;
        }
        else if ("landscape".equals(aStr)) {
            return ScreenOrientation.LANDSCAPE_PRIMARY;
        }
        else if ("portrait-primary".equals(aStr)) {
            return ScreenOrientation.PORTRAIT_PRIMARY;
        }
        else if ("portrait-secondary".equals(aStr)) {
            return ScreenOrientation.PORTRAIT_SECONDARY;
        }
        else if ("landscape-primary".equals(aStr)) {
            return ScreenOrientation.LANDSCAPE_PRIMARY;
        }
        else if ("landscape-secondary".equals(aStr)) {
            return ScreenOrientation.LANDSCAPE_SECONDARY;
        }
        Log.w(LOGTAG, "screenOrientationFromString: unknown orientation string");
        return DEFAULT_SCREEN_ORIENTATION;
    }

    








    public static int screenOrientationToAndroidOrientation(ScreenOrientation aScreenOrientation) {
        switch (aScreenOrientation) {
            case PORTRAIT_PRIMARY:
            case PORTRAIT_SECONDARY:
                return Configuration.ORIENTATION_PORTRAIT;
            case LANDSCAPE_PRIMARY:
            case LANDSCAPE_SECONDARY:
                return Configuration.ORIENTATION_LANDSCAPE;
            case NONE:
            case DEFAULT:
            default:
                return Configuration.ORIENTATION_UNDEFINED;
        }
    }


    









    public static int screenOrientationToActivityInfoOrientation(ScreenOrientation aScreenOrientation) {
        switch (aScreenOrientation) {
            case PORTRAIT_PRIMARY:
                return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
            case PORTRAIT_SECONDARY:
                return ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
            case LANDSCAPE_PRIMARY:
                return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
            case LANDSCAPE_SECONDARY:
                return ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
            case DEFAULT:
            case NONE:
                return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
            default:
                return ActivityInfo.SCREEN_ORIENTATION_NOSENSOR;
        }
    }
}
