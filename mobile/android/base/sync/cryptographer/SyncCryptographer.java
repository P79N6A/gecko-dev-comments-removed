





































package org.mozilla.gecko.sync.cryptographer;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.CryptoInfo;
import org.mozilla.gecko.sync.crypto.Cryptographer;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.cryptographer.CryptoStatusBundle.CryptoStatus;
import java.security.GeneralSecurityException;











public class SyncCryptographer {

  
  private static final String KEY_CIPHER_TEXT =        "ciphertext";
  private static final String KEY_HMAC =               "hmac";
  private static final String KEY_IV =                 "IV";
  private static final String KEY_PAYLOAD =            "payload";
  private static final String KEY_ID =                 "id";
  private static final String KEY_COLLECTION =         "collection";
  private static final String KEY_COLLECTIONS =        "collections";
  private static final String KEY_DEFAULT_COLLECTION = "default";

  private static final String ID_CRYPTO_KEYS =         "keys";
  private static final String CRYPTO_KEYS_COLLECTION = "crypto";

  public String syncKey;
  private String username;
  private KeyBundle keys;

  


  public SyncCryptographer(String username) throws UnsupportedEncodingException {
    this(username, "", "", "");
  }

  public SyncCryptographer(String username, String friendlyBase32SyncKey) throws UnsupportedEncodingException {
    this(username, friendlyBase32SyncKey, "", "");
  }

  public SyncCryptographer(String username, String friendlyBase32SyncKey,
                           String base64EncryptionKey, String base64HmacKey) throws UnsupportedEncodingException {
    this.setUsername(username);
    this.syncKey = friendlyBase32SyncKey;
    this.setKeys(base64EncryptionKey, base64HmacKey);
  }

  


  @SuppressWarnings("unchecked")
  private static final ArrayList<Object> asAList(JSONArray j) {
      return j;
  }

  




  public CryptoStatusBundle encryptWBO(String jsonString) throws CryptoException {
    
    if (keys == null) {
      return new CryptoStatusBundle(CryptoStatus.MISSING_KEYS, jsonString);
    }
    return encrypt(jsonString, keys);
  }

  



  public CryptoStatusBundle decryptWBO(String jsonString) throws CryptoException, UnsupportedEncodingException {
    
    JSONObject json = null;
    JSONObject payload = null;
    try {
      json = getJSONObject(jsonString);
      payload = getJSONObject(json, KEY_PAYLOAD);

    } catch (Exception e) {
      return new CryptoStatusBundle(CryptoStatus.INVALID_JSON, jsonString);
    }

    
    if (!payload.containsKey(KEY_CIPHER_TEXT) ||
        !payload.containsKey(KEY_IV) ||
        !payload.containsKey(KEY_HMAC)) {
      return new CryptoStatusBundle(CryptoStatus.INVALID_JSON, jsonString);
    }

    String id = (String) json.get(KEY_ID);
    if (id.equalsIgnoreCase(ID_CRYPTO_KEYS)) {
      
      return decryptKeysWBO(payload);
    } else if (keys == null) {
      
      return new CryptoStatusBundle(CryptoStatus.MISSING_KEYS, jsonString);
    }

    byte[] clearText = decryptPayload(payload, this.keys);
    return new CryptoStatusBundle(CryptoStatus.OK, new String(clearText));
  }

  








  private CryptoStatusBundle decryptKeysWBO(JSONObject payload) throws CryptoException, UnsupportedEncodingException {
    
    KeyBundle cryptoKeysBundleKeys;
    try {
      cryptoKeysBundleKeys = getCryptoKeysBundleKeys();
    } catch (Exception e) {
      return new CryptoStatusBundle(CryptoStatus.MISSING_SYNCKEY_OR_USER, payload.toString());
    }

    byte[] cryptoKeysBundle = decryptPayload(payload, cryptoKeysBundleKeys);

    
    InputStream stream = new ByteArrayInputStream(cryptoKeysBundle);
    Reader in = new InputStreamReader(stream);
    JSONObject json = null;
    try {
      json = (JSONObject) new JSONParser().parse(in);
    } catch (Exception e) {
      e.printStackTrace();
    }

    if (json == null) {
      throw new CryptoException(new GeneralSecurityException("Could not decrypt JSON payload"));
    }

    
    
    String id = (String) json.get(KEY_ID);
    String collection = (String) json.get(KEY_COLLECTION);

    if (id.equalsIgnoreCase(ID_CRYPTO_KEYS) &&
        collection.equalsIgnoreCase(CRYPTO_KEYS_COLLECTION) &&
        json.containsKey(KEY_DEFAULT_COLLECTION)) {

      
      Object jsonKeysObj = json.get(KEY_DEFAULT_COLLECTION);
      if (jsonKeysObj.getClass() != JSONArray.class) {
        return new CryptoStatusBundle(CryptoStatus.INVALID_KEYS_BUNDLE,
                                      json.toString());
      }

      JSONArray jsonKeys = (JSONArray) jsonKeysObj;
      this.setKeys((String) jsonKeys.get(0), (String) jsonKeys.get(1));

      
      return new CryptoStatusBundle(CryptoStatus.OK,
                                    new String(cryptoKeysBundle));
    } else {
      return new CryptoStatusBundle(CryptoStatus.INVALID_KEYS_BUNDLE,
                                    json.toString());
    }
  }

  








  public CryptoStatusBundle generateCryptoKeysWBOPayload() throws CryptoException, UnsupportedEncodingException {

    
    KeyBundle cryptoKeys = Cryptographer.generateKeys();
    setKeys(new String(Base64.encodeBase64(cryptoKeys.getEncryptionKey())),
            new String(Base64.encodeBase64(cryptoKeys.getHMACKey())));

    
    JSONArray keysArray = new JSONArray();
    asAList(keysArray).add(new String(Base64.encodeBase64(cryptoKeys.getEncryptionKey())));
    asAList(keysArray).add(new String(Base64.encodeBase64(cryptoKeys.getHMACKey())));
    ExtendedJSONObject json = new ExtendedJSONObject();
    json.put(KEY_ID, ID_CRYPTO_KEYS);
    json.put(KEY_COLLECTION, CRYPTO_KEYS_COLLECTION);
    json.put(KEY_COLLECTIONS, "{}");
    json.put(KEY_DEFAULT_COLLECTION, keysArray);

    
    KeyBundle cryptoKeysBundleKeys;
    try {
      cryptoKeysBundleKeys = getCryptoKeysBundleKeys();
    } catch (Exception e) {
      return new CryptoStatusBundle(CryptoStatus.MISSING_SYNCKEY_OR_USER, "");
    }

    return encrypt(json.toString(), cryptoKeysBundleKeys);
  }

  

  






  private CryptoStatusBundle encrypt(String message, KeyBundle keys) throws CryptoException {
    CryptoInfo encrypted = Cryptographer.encrypt(new CryptoInfo(message.getBytes(), keys));
    String payload = createJSONBundle(encrypted);
    return new CryptoStatusBundle(CryptoStatus.OK, payload);
  }

  








  private byte[] decryptPayload(JSONObject payload, KeyBundle keybundle) throws CryptoException, UnsupportedEncodingException {
    byte[] clearText = Cryptographer.decrypt(
      new CryptoInfo (
        Base64.decodeBase64(((String) payload.get(KEY_CIPHER_TEXT)).getBytes("UTF-8")),
        Base64.decodeBase64(((String) payload.get(KEY_IV)).getBytes("UTF-8")),
        Utils.hex2Byte( (String) payload.get(KEY_HMAC) ),
        keybundle
        )
      );

    return clearText;
  }

  





  private JSONObject getJSONObject(String jsonString) throws Exception {
    Reader in = new StringReader(jsonString);
    try {
      return (JSONObject) new JSONParser().parse(in);
    } catch (Exception e) {
      throw e;
    }
  }

  







  private JSONObject getJSONObject(JSONObject json, String key) throws Exception {
    try {
      return getJSONObject((String) json.get(key));
    } catch (Exception e) {
      throw e;
    }
  }

  


  private String createJSONBundle(CryptoInfo info) {
    ExtendedJSONObject json = new ExtendedJSONObject();
    json.put(KEY_CIPHER_TEXT, new String(Base64.encodeBase64(info.getMessage())));
    json.put(KEY_IV,          new String(Base64.encodeBase64(info.getIV())));
    json.put(KEY_HMAC,        Utils.byte2hex(info.getHMAC()));
    return json.toString();
  }

  


  public KeyBundle getCryptoKeysBundleKeys() {
    return new KeyBundle(username, syncKey);
  }

  public KeyBundle getKeys() {
    return keys;
  }

  


  public void setKeys(String base64EncryptionKey, String base64HmacKey) throws UnsupportedEncodingException {
    this.keys = new KeyBundle(Base64.decodeBase64(base64EncryptionKey.getBytes("UTF-8")),
                              Base64.decodeBase64(base64HmacKey.getBytes("UTF-8")));
  }

  public String getUsername() {
    return username;
  }

  public void setUsername(String username) {
    this.username = username;
  }
}
