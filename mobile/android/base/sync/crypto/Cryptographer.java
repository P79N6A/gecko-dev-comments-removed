





































package org.mozilla.gecko.sync.crypto;

import java.io.UnsupportedEncodingException;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.KeyGenerator;
import javax.crypto.Mac;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import org.mozilla.apache.commons.codec.binary.Base32;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.sync.Utils;




public class Cryptographer {

  private static final String TRANSFORMATION     = "AES/CBC/PKCS5Padding";
  private static final String KEY_ALGORITHM_SPEC = "AES";
  private static final int    KEY_SIZE           = 256;

  public static CryptoInfo encrypt(CryptoInfo info) throws CryptoException {

    Cipher cipher = getCipher();
    try {
      byte[] encryptionKey = info.getKeys().getEncryptionKey();
      SecretKeySpec spec = new SecretKeySpec(encryptionKey, KEY_ALGORITHM_SPEC);

      
      if (info.getIV() == null ||
          info.getIV().length == 0) {
        cipher.init(Cipher.ENCRYPT_MODE, spec);
      } else {
        System.out.println("IV is " + info.getIV().length);
        cipher.init(Cipher.ENCRYPT_MODE, spec, new IvParameterSpec(info.getIV()));
      }
    } catch (GeneralSecurityException ex) {
      ex.printStackTrace();
      throw new CryptoException(ex);
    }

    
    byte[] encryptedBytes = commonCrypto(cipher, info.getMessage());
    info.setMessage(encryptedBytes);

    
    info.setIV(cipher.getIV());

    
    info.setHMAC(generateHMAC(info));

    return info;

  }

  








  public static byte[] decrypt(CryptoInfo info) throws CryptoException {

    
    if (!verifyHMAC(info)) {
      throw new HMACVerificationException();
    }

    Cipher cipher = getCipher();
    try {
      byte[] encryptionKey = info.getKeys().getEncryptionKey();
      SecretKeySpec spec = new SecretKeySpec(encryptionKey, KEY_ALGORITHM_SPEC);
      cipher.init(Cipher.DECRYPT_MODE, spec, new IvParameterSpec(info.getIV()));
    } catch (GeneralSecurityException ex) {
      ex.printStackTrace();
      throw new CryptoException(ex);
    }
    return commonCrypto(cipher, info.getMessage());
  }

  


  public static KeyBundle generateKeys() throws CryptoException {
    KeyGenerator keygen;
    try {
      keygen = KeyGenerator.getInstance(KEY_ALGORITHM_SPEC);
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
      throw new CryptoException(e);
    }

    keygen.init(KEY_SIZE);
    byte[] encryptionKey = keygen.generateKey().getEncoded();
    byte[] hmacKey = keygen.generateKey().getEncoded();
    return new KeyBundle(encryptionKey, hmacKey);
  }

  






  private static byte[] commonCrypto(Cipher cipher, byte[] inputMessage)
                        throws CryptoException {
    byte[] outputMessage = null;
    try {
      outputMessage = cipher.doFinal(inputMessage);
    } catch (IllegalBlockSizeException e) {
      e.printStackTrace();
      throw new CryptoException(e);
    } catch (BadPaddingException e) {
      e.printStackTrace();
      throw new CryptoException(e);
    }
    return outputMessage;
  }

  




  private static Cipher getCipher() throws CryptoException {
    Cipher cipher = null;
    try {
      cipher = Cipher.getInstance(TRANSFORMATION);
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
      throw new CryptoException(e);
    } catch (NoSuchPaddingException e) {
      e.printStackTrace();
      throw new CryptoException(e);
    }
    return cipher;
  }

  


  private static boolean verifyHMAC(CryptoInfo bundle) {
    byte[] generatedHMAC = generateHMAC(bundle);
    byte[] expectedHMAC  = bundle.getHMAC();
    boolean eq = Arrays.equals(generatedHMAC, expectedHMAC);
    if (!eq) {
      System.err.println("Failed HMAC verification.");
      System.err.println("Expecting: " + Utils.byte2hex(generatedHMAC));
      System.err.println("Got:       " + Utils.byte2hex(expectedHMAC));
    }
    return eq;
  }

  



  private static byte[] generateHMAC(CryptoInfo bundle) {
    Mac hmacHasher = HKDF.makeHMACHasher(bundle.getKeys().getHMACKey());
    return hmacHasher.doFinal(Base64.encodeBase64(bundle.getMessage()));
  }

  public static byte[] sha1(String utf8) throws NoSuchAlgorithmException, UnsupportedEncodingException {
    MessageDigest sha1 = MessageDigest.getInstance("SHA-1");
    return sha1.digest(utf8.getBytes("UTF-8"));
  }

  public static String sha1Base32(String utf8) throws NoSuchAlgorithmException, UnsupportedEncodingException {
    return new Base32().encodeAsString(sha1(utf8)).toLowerCase();
  }
}
