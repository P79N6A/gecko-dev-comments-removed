





































package org.mozilla.gecko.sync.crypto;

import java.io.UnsupportedEncodingException;
import java.security.NoSuchAlgorithmException;

import javax.crypto.Mac;

import org.mozilla.apache.commons.codec.binary.Base64;

public class KeyBundle {

    private byte[] encryptionKey;
    private byte[] hmacKey;

    
    private static final byte[] EMPTY_BYTES      = {};
    private static final byte[] ENCR_INPUT_BYTES = {1};
    private static final byte[] HMAC_INPUT_BYTES = {2};

    









    public static String usernameFromAccount(String account) throws NoSuchAlgorithmException, UnsupportedEncodingException {
      if (account == null || account.equals("")) {
        throw new IllegalArgumentException("No account name provided.");
      }
      if (account.matches("^[A-Za-z0-9._-]+$")) {
        return account;
      }
      return Cryptographer.sha1Base32(account);
    }

    
    

    






    public KeyBundle(String username, String base32SyncKey) {
      if (base32SyncKey == null) {
        throw new IllegalArgumentException("No sync key provided.");
      }
      if (username == null || username.equals("")) {
        throw new IllegalArgumentException("No username provided.");
      }
      
      try {
        username = usernameFromAccount(username);
      } catch (NoSuchAlgorithmException e) {
        throw new IllegalArgumentException("Invalid username.");
      } catch (UnsupportedEncodingException e) {
        throw new IllegalArgumentException("Invalid username.");
      }

      byte[] syncKey = Utils.decodeFriendlyBase32(base32SyncKey);
      byte[] user    = username.getBytes();

      Mac hmacHasher = HKDF.makeHMACHasher(syncKey);

      byte[] encrBytes = Utils.concatAll(EMPTY_BYTES, HKDF.HMAC_INPUT, user, ENCR_INPUT_BYTES);
      byte[] encrKey   = HKDF.digestBytes(encrBytes, hmacHasher);
      byte[] hmacBytes = Utils.concatAll(encrKey, HKDF.HMAC_INPUT, user, HMAC_INPUT_BYTES);

      this.hmacKey       = HKDF.digestBytes(hmacBytes, hmacHasher);
      this.encryptionKey = encrKey;
    }

    public KeyBundle(byte[] encryptionKey, byte[] hmacKey) {
       this.setEncryptionKey(encryptionKey);
       this.setHMACKey(hmacKey);
    }

    public byte[] getEncryptionKey() {
        return encryptionKey;
    }

    public void setEncryptionKey(byte[] encryptionKey) {
        this.encryptionKey = encryptionKey;
    }

    public byte[] getHMACKey() {
        return hmacKey;
    }

    public void setHMACKey(byte[] hmacKey) {
        this.hmacKey = hmacKey;
    }

    public static KeyBundle decodeKeyStrings(String base64EncryptionKey, String base64HmacKey) throws UnsupportedEncodingException {
      return new KeyBundle(Base64.decodeBase64(base64EncryptionKey.getBytes("UTF-8")),
                           Base64.decodeBase64(base64HmacKey.getBytes("UTF-8")));
    }
}
