



package org.mozilla.gecko.background.nativecode;

import java.security.GeneralSecurityException;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.mozglue.RobocopTarget;

import android.util.Log;

@RobocopTarget
public class NativeCrypto {
  static {
    try {
      System.loadLibrary("mozglue");
    } catch (UnsatisfiedLinkError e) {
      Log.wtf("NativeCrypto", "Couldn't load mozglue. Trying /data/app-lib path.");
      try {
        System.load("/data/app-lib/" + AppConstants.ANDROID_PACKAGE_NAME + "/libmozglue.so");
      } catch (Throwable ee) {
          try {
            Log.wtf("NativeCrypto", "Couldn't load mozglue: " + ee + ". Trying /data/data path.");
            System.load("/data/data/" + AppConstants.ANDROID_PACKAGE_NAME + "/lib/libmozglue.so");
          } catch (UnsatisfiedLinkError eee) {
              Log.wtf("NativeCrypto", "Failed every attempt to load mozglue. Giving up.");
              throw new RuntimeException("Unable to load mozglue", eee);
          }
      }
    }
  }

  


  public native static byte[] pbkdf2SHA256(byte[] password, byte[] salt, int c, int dkLen)
      throws GeneralSecurityException;

  


  public native static byte[] sha1(byte[] str);
}
