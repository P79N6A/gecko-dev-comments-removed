



package org.mozilla.gecko.tests.helpers;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertTrue;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.tests.UITestContext;

import android.app.Activity;
import android.os.Build;
import android.util.DisplayMetrics;

import com.robotium.solo.Solo;





public final class DeviceHelper {
    public enum Type {
        PHONE,
        TABLET
    }

    public enum AndroidVersion {
        v2x,
        v3x,
        v4x
    }

    private static Activity sActivity;
    private static Solo sSolo;

    private static Type sDeviceType;
    private static AndroidVersion sAndroidVersion;

    private static int sScreenHeight;
    private static int sScreenWidth;

    private DeviceHelper() {  }

    public static void assertIsTablet() {
        fAssertTrue("The device is a tablet", isTablet());
    }

    protected static void init(final UITestContext context) {
        sActivity = context.getActivity();
        sSolo = context.getSolo();

        setAndroidVersion();
        setScreenDimensions();
        setDeviceType();
    }

    private static void setAndroidVersion() {
        int sdk = Build.VERSION.SDK_INT;
        if (sdk < Build.VERSION_CODES.HONEYCOMB) {
            sAndroidVersion = AndroidVersion.v2x;
        } else if (sdk > Build.VERSION_CODES.HONEYCOMB_MR2) {
            sAndroidVersion = AndroidVersion.v4x;
        } else {
            sAndroidVersion = AndroidVersion.v3x;
        }
    }

    private static void setScreenDimensions() {
        final DisplayMetrics dm = new DisplayMetrics();
        sActivity.getWindowManager().getDefaultDisplay().getMetrics(dm);

        sScreenHeight = dm.heightPixels;
        sScreenWidth = dm.widthPixels;
    }

    private static void setDeviceType() {
        sDeviceType = (GeckoAppShell.isTablet() ? Type.TABLET : Type.PHONE);
    }

    public static int getScreenHeight() {
        return sScreenHeight;
    }

    public static int getScreenWidth() {
        return sScreenWidth;
    }

    public static AndroidVersion getAndroidVersion() {
        return sAndroidVersion;
    }

    public static boolean isPhone() {
        return (sDeviceType == Type.PHONE);
    }

    public static boolean isTablet() {
        return (sDeviceType == Type.TABLET);
    }

    public static void setLandscapeRotation() {
        sSolo.setActivityOrientation(Solo.LANDSCAPE);
    }

    public static void setPortraitOrientation() {
        sSolo.setActivityOrientation(Solo.LANDSCAPE);
    }
}
