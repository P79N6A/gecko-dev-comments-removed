



package org.mozilla.gecko.background.nativecode;

import org.mozilla.gecko.mozglue.RobocopTarget;

import java.security.GeneralSecurityException;

@RobocopTarget
public class NativeCrypto {
  static {
    System.loadLibrary("mozglue");
  }

  


  public native static byte[] pbkdf2SHA256(byte[] password, byte[] salt, int c, int dkLen)
      throws GeneralSecurityException;

  


  public native static byte[] sha1(byte[] str);
}
