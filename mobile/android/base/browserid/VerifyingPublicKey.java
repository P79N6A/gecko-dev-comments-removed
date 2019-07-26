



package org.mozilla.gecko.browserid;

import java.security.GeneralSecurityException;


public interface VerifyingPublicKey {
  






  public String serialize();

  










  public boolean verifyMessage(byte[] message, byte[] signature) throws GeneralSecurityException;
}
