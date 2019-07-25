



package org.mozilla.gecko.sync.crypto;

import java.security.GeneralSecurityException;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.HashMap;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.Mac;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import org.mozilla.apache.commons.codec.binary.Base64;




public class CryptoInfo {
  private static final String TRANSFORMATION     = "AES/CBC/PKCS5Padding";
  private static final String KEY_ALGORITHM_SPEC = "AES";

  private byte[] message;
  private byte[] iv;
  private byte[] hmac;
  private KeyBundle keys;

  


  public static CryptoInfo encrypt(byte[] plaintextBytes, KeyBundle keys) throws CryptoException {
    CryptoInfo info = new CryptoInfo(plaintextBytes, keys);
    info.encrypt();
    return info;
  }

  


  public static CryptoInfo encrypt(byte[] plaintextBytes, byte[] iv, KeyBundle keys) throws CryptoException {
    CryptoInfo info = new CryptoInfo(plaintextBytes, iv, null, keys);
    info.encrypt();
    return info;
  }

  


  public static CryptoInfo decrypt(byte[] ciphertext, byte[] iv, byte[] hmac, KeyBundle keys) throws CryptoException {
    CryptoInfo info = new CryptoInfo(ciphertext, iv, hmac, keys);
    info.decrypt();
    return info;
  }

  


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

  


  public static byte[] generatedHMACFor(byte[] message, KeyBundle keys) throws NoSuchAlgorithmException, InvalidKeyException {
    Mac hmacHasher = HKDF.makeHMACHasher(keys.getHMACKey());
    return hmacHasher.doFinal(Base64.encodeBase64(message));
  }

  


  public boolean generatedHMACIsHMAC() throws NoSuchAlgorithmException, InvalidKeyException {
    byte[] generatedHMAC = generatedHMACFor(getMessage(), getKeys());
    byte[] expectedHMAC  = getHMAC();
    return Arrays.equals(generatedHMAC, expectedHMAC);
  }

  







  private static byte[] commonCrypto(Cipher cipher, byte[] inputMessage)
                        throws CryptoException {
    byte[] outputMessage = null;
    try {
      outputMessage = cipher.doFinal(inputMessage);
    } catch (IllegalBlockSizeException e) {
      throw new CryptoException(e);
    } catch (BadPaddingException e) {
      throw new CryptoException(e);
    }
    return outputMessage;
  }

  




  public void encrypt() throws CryptoException {

    Cipher cipher = CryptoInfo.getCipher(TRANSFORMATION);
    try {
      byte[] encryptionKey = getKeys().getEncryptionKey();
      SecretKeySpec spec = new SecretKeySpec(encryptionKey, KEY_ALGORITHM_SPEC);

      
      if (getIV() == null || getIV().length == 0) {
        cipher.init(Cipher.ENCRYPT_MODE, spec);
      } else {
        cipher.init(Cipher.ENCRYPT_MODE, spec, new IvParameterSpec(getIV()));
      }
    } catch (GeneralSecurityException ex) {
      throw new CryptoException(ex);
    }

    
    byte[] encryptedBytes = commonCrypto(cipher, getMessage());
    byte[] iv = cipher.getIV();

    byte[] hmac;
    
    try {
      hmac = generatedHMACFor(encryptedBytes, keys);
    } catch (NoSuchAlgorithmException e) {
      throw new CryptoException(e);
    } catch (InvalidKeyException e) {
      throw new CryptoException(e);
    }

    
    this.setHMAC(hmac);
    this.setIV(iv);
    this.setMessage(encryptedBytes);
  }

  




  public void decrypt() throws CryptoException {

    
    try {
      if (!generatedHMACIsHMAC()) {
        throw new HMACVerificationException();
      }
    } catch (NoSuchAlgorithmException e) {
      throw new CryptoException(e);
    } catch (InvalidKeyException e) {
      throw new CryptoException(e);
    }

    Cipher cipher = CryptoInfo.getCipher(TRANSFORMATION);
    try {
      byte[] encryptionKey = getKeys().getEncryptionKey();
      SecretKeySpec spec = new SecretKeySpec(encryptionKey, KEY_ALGORITHM_SPEC);
      cipher.init(Cipher.DECRYPT_MODE, spec, new IvParameterSpec(getIV()));
    } catch (GeneralSecurityException ex) {
      throw new CryptoException(ex);
    }
    byte[] decryptedBytes = commonCrypto(cipher, getMessage());
    byte[] iv = cipher.getIV();

    
    this.setHMAC(null);
    this.setIV(iv);
    this.setMessage(decryptedBytes);
  }

  




  private static Cipher getCipher(String transformation) throws CryptoException {
    try {
      return Cipher.getInstance(transformation);
    } catch (NoSuchAlgorithmException e) {
      throw new CryptoException(e);
    } catch (NoSuchPaddingException e) {
      throw new CryptoException(e);
    }
  }
}
