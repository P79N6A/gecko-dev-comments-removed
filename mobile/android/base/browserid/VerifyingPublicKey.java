



package org.mozilla.gecko.browserid;

import java.security.GeneralSecurityException;

import org.mozilla.gecko.sync.ExtendedJSONObject;


public interface VerifyingPublicKey {
  






  public ExtendedJSONObject toJSONObject();

  










  public boolean verifyMessage(byte[] message, byte[] signature) throws GeneralSecurityException;
}
