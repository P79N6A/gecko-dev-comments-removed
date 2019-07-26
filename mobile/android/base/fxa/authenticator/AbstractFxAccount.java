



package org.mozilla.gecko.fxa.authenticator;

import java.security.GeneralSecurityException;

import org.mozilla.gecko.browserid.BrowserIDKeyPair;


























public interface AbstractFxAccount {
  



  public String getServerURI();

  public boolean isValid();
  public void setInvalid();

  public byte[] getSessionToken();
  public byte[] getKeyFetchToken();

  public void setSessionToken(byte[] token);
  public void setKeyFetchToken(byte[] token);

  




  public boolean isVerified();

  



  public void setVerified();

  public byte[] getKa();
  public void setKa(byte[] kA);

  public byte[] getKb();

  









  public void setWrappedKb(byte[] wrappedKb);

  BrowserIDKeyPair getAssertionKeyPair() throws GeneralSecurityException;

  public String getCertificate();
  public void setCertificate(String certificate);

  public String getAssertion();
  public void setAssertion(String assertion);

  public byte[] getEmailUTF8();

  public byte[] getQuickStretchedPW();
  public void setQuickStretchedPW(byte[] quickStretchedPW);
}
