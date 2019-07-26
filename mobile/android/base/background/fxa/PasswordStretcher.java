



package org.mozilla.gecko.background.fxa;

import java.io.UnsupportedEncodingException;
import java.security.GeneralSecurityException;

public interface PasswordStretcher {
  public byte[] getQuickStretchedPW(byte[] emailUTF8) throws UnsupportedEncodingException, GeneralSecurityException;
}
