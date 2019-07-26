



package org.mozilla.gecko.browserid;

import java.security.GeneralSecurityException;

public interface SigningPrivateKey {
  







  public String getAlgorithm();

  









  public String serialize();

  





  public byte[] signMessage(byte[] message) throws GeneralSecurityException;
}
