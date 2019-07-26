



package org.mozilla.gecko.browserid;

public class BrowserIDKeyPair {
  protected final SigningPrivateKey privateKey;
  protected final VerifyingPublicKey publicKey;

  public BrowserIDKeyPair(SigningPrivateKey privateKey, VerifyingPublicKey publicKey) {
    this.privateKey = privateKey;
    this.publicKey = publicKey;
  }

  public SigningPrivateKey getPrivate() {
    return this.privateKey;
  }

  public VerifyingPublicKey getPublic() {
    return this.publicKey;
  }
}
