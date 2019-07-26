



package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.GeckoLoader;

import android.content.Context;
import org.mozilla.gecko.mozglue.RobocopTarget;

public class NSSBridge {
    private static final String LOGTAG = "NSSBridge";

    private static native String nativeEncrypt(String aDb, String aValue);
    private static native String nativeDecrypt(String aDb, String aValue);

    @RobocopTarget
    static public String encrypt(Context context, String aValue)
      throws Exception {
        String resourcePath = context.getPackageResourcePath();
        GeckoLoader.loadNSSLibs(context, resourcePath);

        String path = GeckoProfile.get(context).getDir().toString();
        return nativeEncrypt(path, aValue);
    }

    @RobocopTarget
    static public String encrypt(Context context, String profilePath, String aValue)
      throws Exception {
        String resourcePath = context.getPackageResourcePath();
        GeckoLoader.loadNSSLibs(context, resourcePath);

        return nativeEncrypt(profilePath, aValue);
    }

    @RobocopTarget
    static public String decrypt(Context context, String aValue)
      throws Exception {
        String resourcePath = context.getPackageResourcePath();
        GeckoLoader.loadNSSLibs(context, resourcePath);

        String path = GeckoProfile.get(context).getDir().toString();
        return nativeDecrypt(path, aValue);
    }

    @RobocopTarget
    static public String decrypt(Context context, String profilePath, String aValue)
      throws Exception {
        String resourcePath = context.getPackageResourcePath();
        GeckoLoader.loadNSSLibs(context, resourcePath);

        return nativeDecrypt(profilePath, aValue);
    }
}
