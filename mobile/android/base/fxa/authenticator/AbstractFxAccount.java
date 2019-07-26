



package org.mozilla.gecko.fxa.authenticator;

import java.security.GeneralSecurityException;

import org.mozilla.gecko.browserid.BrowserIDKeyPair;


























public interface AbstractFxAccount {
  



  public String getServerURI();

  public byte[] getSessionToken();
  public byte[] getKeyFetchToken();

  public void invalidateSessionToken();
  public void invalidateKeyFetchToken();

  




  public boolean isVerified();

  



  public void setVerified();

  public byte[] getKa();
  public void setKa(byte[] kA);

  public byte[] getKb();

  









  public void setWrappedKb(byte[] wrappedKb);

  BrowserIDKeyPair getAssertionKeyPair() throws GeneralSecurityException;
}
