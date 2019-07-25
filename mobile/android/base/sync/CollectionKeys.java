



package org.mozilla.gecko.sync;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map.Entry;
import java.util.Set;

import org.json.simple.JSONArray;
import org.json.simple.parser.ParseException;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;

public class CollectionKeys {
  private KeyBundle                  defaultKeyBundle     = null;
  private HashMap<String, KeyBundle> collectionKeyBundles = new HashMap<String, KeyBundle>();

  



  public static CollectionKeys generateCollectionKeys() throws CryptoException {
    CollectionKeys ck = new CollectionKeys();
    ck.clear();
    ck.defaultKeyBundle = KeyBundle.withRandomKeys();
    
    
    return ck;
  }

  public KeyBundle defaultKeyBundle() throws NoCollectionKeysSetException {
    if (this.defaultKeyBundle == null) {
      throw new NoCollectionKeysSetException();
    }
    return this.defaultKeyBundle;
  }

  public boolean keyBundleForCollectionIsNotDefault(String collection) {
    return collectionKeyBundles.containsKey(collection);
  }

  public KeyBundle keyBundleForCollection(String collection)
      throws NoCollectionKeysSetException {
    if (this.defaultKeyBundle == null) {
      throw new NoCollectionKeysSetException();
    }
    if (keyBundleForCollectionIsNotDefault(collection)) {
      return collectionKeyBundles.get(collection);
    }
    return this.defaultKeyBundle;
  }

  



  private static KeyBundle arrayToKeyBundle(JSONArray array) throws UnsupportedEncodingException {
    String encKeyStr  = (String) array.get(0);
    String hmacKeyStr = (String) array.get(1);
    return KeyBundle.fromBase64EncodedKeys(encKeyStr, hmacKeyStr);
  }

  @SuppressWarnings("unchecked")
  private static JSONArray keyBundleToArray(KeyBundle bundle) {
    
    JSONArray keysArray = new JSONArray();
    keysArray.add(new String(Base64.encodeBase64(bundle.getEncryptionKey())));
    keysArray.add(new String(Base64.encodeBase64(bundle.getHMACKey())));
    return keysArray;
  }

  private ExtendedJSONObject asRecordContents() throws NoCollectionKeysSetException {
    ExtendedJSONObject json = new ExtendedJSONObject();
    json.put("id", "keys");
    json.put("collection", "crypto");
    json.put("default", keyBundleToArray(this.defaultKeyBundle()));
    ExtendedJSONObject colls = new ExtendedJSONObject();
    for (Entry<String, KeyBundle> collKey : collectionKeyBundles.entrySet()) {
      colls.put(collKey.getKey(), keyBundleToArray(collKey.getValue()));
    }
    json.put("collections", colls);
    return json;
  }

  public CryptoRecord asCryptoRecord() throws NoCollectionKeysSetException {
    ExtendedJSONObject payload = this.asRecordContents();
    CryptoRecord record = new CryptoRecord(payload);
    record.collection = "crypto";
    record.guid       = "keys";
    record.deleted    = false;
    return record;
  }

  









  public void setKeyPairsFromWBO(CryptoRecord keys, KeyBundle syncKeyBundle)
      throws CryptoException, IOException, ParseException, NonObjectJSONException {
    if (syncKeyBundle != null) {
      keys.keyBundle = syncKeyBundle;
      keys.decrypt();
    }
    ExtendedJSONObject cleartext = keys.payload;
    KeyBundle defaultKey = arrayToKeyBundle((JSONArray) cleartext.get("default"));

    ExtendedJSONObject collections = cleartext.getObject("collections");
    HashMap<String, KeyBundle> collectionKeys = new HashMap<String, KeyBundle>();
    for (Entry<String, Object> pair : collections.entryIterable()) {
      KeyBundle bundle = arrayToKeyBundle((JSONArray) pair.getValue());
      collectionKeys.put(pair.getKey(), bundle);
    }

    this.collectionKeyBundles = collectionKeys;
    this.defaultKeyBundle     = defaultKey;
  }

  public void setKeyBundleForCollection(String collection, KeyBundle keys) {
    this.collectionKeyBundles.put(collection, keys);
  }

  public void setDefaultKeyBundle(KeyBundle keys) {
    this.defaultKeyBundle = keys;
  }

  public void clear() {
    this.defaultKeyBundle = null;
    this.collectionKeyBundles = new HashMap<String, KeyBundle>();
  }

  





  public static Set<String> differences(CollectionKeys a, CollectionKeys b) {
    Set<String> differences = new HashSet<String>();

    
    for (String collection : a.collectionKeyBundles.keySet()) {
      KeyBundle key = b.collectionKeyBundles.get(collection);
      if (key == null) {
        differences.add(collection);
        continue;
      }
      if (!key.equals(a.collectionKeyBundles.get(collection))) {
        differences.add(collection);
      }
    }

    
    for (String collection : b.collectionKeyBundles.keySet()) {
      if (!a.collectionKeyBundles.containsKey(collection)) {
        differences.add(collection);
      }
    }

    return differences;
  }
}
