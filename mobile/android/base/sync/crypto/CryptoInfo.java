



package org.mozilla.gecko.sync.crypto;




public class CryptoInfo {

  private byte[] message;
  private byte[] iv;
  private byte[] hmac;
  private KeyBundle keys;

  


  public CryptoInfo(byte[] message, KeyBundle keys) {
    this.setMessage(message);
    this.setKeys(keys);
  }

  


  public CryptoInfo(byte[] message, byte[] iv, byte[] hmac, KeyBundle keys) {
    this.setMessage(message);
    this.setIV(iv);
    this.setHMAC(hmac);
    this.setKeys(keys);
  }

  public byte[] getMessage() {
    return message;
  }

  public void setMessage(byte[] message) {
    this.message = message;
  }

  public byte[] getIV() {
    return iv;
  }

  public void setIV(byte[] iv) {
    this.iv = iv;
  }

  public byte[] getHMAC() {
    return hmac;
  }

  public void setHMAC(byte[] hmac) {
    this.hmac = hmac;
  }

  public KeyBundle getKeys() {
    return keys;
  }

  public void setKeys(KeyBundle keys) {
    this.keys = keys;
  }
}
