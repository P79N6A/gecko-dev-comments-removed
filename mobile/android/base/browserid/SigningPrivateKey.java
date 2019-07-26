



package org.mozilla.gecko.browserid;

import java.security.GeneralSecurityException;

import org.mozilla.gecko.sync.ExtendedJSONObject;

public interface SigningPrivateKey {
  







  public String getAlgorithm();

  









  public ExtendedJSONObject toJSONObject();

  





  public byte[] signMessage(byte[] message) throws GeneralSecurityException;
}
