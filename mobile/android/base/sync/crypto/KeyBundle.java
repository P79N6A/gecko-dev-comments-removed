



package org.mozilla.gecko.sync.crypto;

import java.io.UnsupportedEncodingException;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

import javax.crypto.KeyGenerator;
import javax.crypto.Mac;

import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.sync.Utils;

public class KeyBundle {
    private static final String KEY_ALGORITHM_SPEC = "AES";
    private static final int    KEY_SIZE           = 256;

    private byte[] encryptionKey;
    private byte[] hmacKey;

    
    private static final byte[] EMPTY_BYTES      = {};
    private static final byte[] ENCR_INPUT_BYTES = {1};
    private static final byte[] HMAC_INPUT_BYTES = {2};

    






    public KeyBundle(String username, String base32SyncKey) throws CryptoException {
      if (base32SyncKey == null) {
        throw new IllegalArgumentException("No sync key provided.");
      }
      if (username == null || username.equals("")) {
        throw new IllegalArgumentException("No username provided.");
      }
      
      try {
        username = Utils.usernameFromAccount(username);
      } catch (NoSuchAlgorithmException | UnsupportedEncodingException e) {
        throw new IllegalArgumentException("Invalid username.");
      }

      byte[] syncKey = Utils.decodeFriendlyBase32(base32SyncKey);
      byte[] user    = username.getBytes();

      Mac hmacHasher;
      try {
        hmacHasher = HKDF.makeHMACHasher(syncKey);
      } catch (NoSuchAlgorithmException | InvalidKeyException e) {
        throw new CryptoException(e);
      }
      assert(hmacHasher != null); 

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

    




    public static KeyBundle fromBase64EncodedKeys(String base64EncryptionKey, String base64HmacKey) throws UnsupportedEncodingException {
      return new KeyBundle(Base64.decodeBase64(base64EncryptionKey.getBytes("UTF-8")),
                           Base64.decodeBase64(base64HmacKey.getBytes("UTF-8")));
    }

    




    public static KeyBundle withRandomKeys() throws CryptoException {
      KeyGenerator keygen;
      try {
        keygen = KeyGenerator.getInstance(KEY_ALGORITHM_SPEC);
      } catch (NoSuchAlgorithmException e) {
        throw new CryptoException(e);
      }

      keygen.init(KEY_SIZE);
      byte[] encryptionKey = keygen.generateKey().getEncoded();
      byte[] hmacKey = keygen.generateKey().getEncoded();

      return new KeyBundle(encryptionKey, hmacKey);
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

    @Override
    public boolean equals(Object o) {
      if (!(o instanceof KeyBundle)) {
        return false;
      }
      KeyBundle other = (KeyBundle) o;
      return Arrays.equals(other.encryptionKey, this.encryptionKey) &&
             Arrays.equals(other.hmacKey, this.hmacKey);
    }

    @Override
    public int hashCode() {
      throw new UnsupportedOperationException("No hashCode for KeyBundle.");
    }
}
