



package org.mozilla.gecko.sync;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

import org.json.simple.JSONObject;
import org.json.simple.parser.ParseException;
import org.mozilla.apache.commons.codec.binary.Base64;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.CryptoInfo;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.crypto.MissingCryptoInputException;
import org.mozilla.gecko.sync.crypto.NoKeyBundleException;
import org.mozilla.gecko.sync.repositories.domain.Record;



















public class CryptoRecord extends Record {

  
  private static final String KEY_ID         = "id";
  private static final String KEY_COLLECTION = "collection";
  private static final String KEY_PAYLOAD    = "payload";
  private static final String KEY_MODIFIED   = "modified";
  private static final String KEY_SORTINDEX  = "sortindex";
  private static final String KEY_TTL        = "ttl";
  private static final String KEY_CIPHERTEXT = "ciphertext";
  private static final String KEY_HMAC       = "hmac";
  private static final String KEY_IV         = "IV";

  







  private static byte[] decryptPayload(ExtendedJSONObject payload, KeyBundle keybundle) throws CryptoException, UnsupportedEncodingException {
    byte[] ciphertext = Base64.decodeBase64(((String) payload.get(KEY_CIPHERTEXT)).getBytes("UTF-8"));
    byte[] iv         = Base64.decodeBase64(((String) payload.get(KEY_IV)).getBytes("UTF-8"));
    byte[] hmac       = Utils.hex2Byte((String) payload.get(KEY_HMAC));

    return CryptoInfo.decrypt(ciphertext, iv, hmac, keybundle).getMessage();
  }

  
  

  public ExtendedJSONObject payload;
  public KeyBundle   keyBundle;

  


  public CryptoRecord() {
    super(null, null, 0, false);
  }

  public CryptoRecord(ExtendedJSONObject payload) {
    super(null, null, 0, false);
    if (payload == null) {
      throw new IllegalArgumentException(
          "No payload provided to CryptoRecord constructor.");
    }
    this.payload = payload;
  }

  public CryptoRecord(String jsonString) throws IOException, ParseException, NonObjectJSONException {
    this(ExtendedJSONObject.parseJSONObject(jsonString));
  }

  




  public CryptoRecord(Record source) {
    super(source.guid, source.collection, source.lastModified, source.deleted);
    this.ttl = source.ttl;
  }

  @Override
  public Record copyWithIDs(String guid, long androidID) {
    CryptoRecord out = new CryptoRecord(this);
    out.guid         = guid;
    out.androidID    = androidID;
    out.sortIndex    = this.sortIndex;
    out.ttl          = this.ttl;
    out.payload      = (this.payload == null) ? null : new ExtendedJSONObject(this.payload.object);
    out.keyBundle    = this.keyBundle;    
    return out;
  }

  














  public static CryptoRecord fromJSONRecord(String jsonRecord)
      throws ParseException, NonObjectJSONException, IOException {
    byte[] bytes = jsonRecord.getBytes("UTF-8");
    ExtendedJSONObject object = ExtendedJSONObject.parseUTF8AsJSONObject(bytes);

    return CryptoRecord.fromJSONRecord(object);
  }

  
  public static CryptoRecord fromJSONRecord(ExtendedJSONObject jsonRecord)
      throws IOException, ParseException, NonObjectJSONException {
    String id                  = (String) jsonRecord.get(KEY_ID);
    String collection          = (String) jsonRecord.get(KEY_COLLECTION);
    String jsonEncodedPayload  = (String) jsonRecord.get(KEY_PAYLOAD);

    ExtendedJSONObject payload = ExtendedJSONObject.parseJSONObject(jsonEncodedPayload);

    CryptoRecord record = new CryptoRecord(payload);
    record.guid         = id;
    record.collection   = collection;
    if (jsonRecord.containsKey(KEY_MODIFIED)) {
      record.lastModified = jsonRecord.getTimestamp(KEY_MODIFIED);
    }
    if (jsonRecord.containsKey(KEY_SORTINDEX)) {
      record.sortIndex = jsonRecord.getLong(KEY_SORTINDEX);
    }
    if (jsonRecord.containsKey(KEY_TTL)) {
      
      
      record.ttl = jsonRecord.getLong(KEY_TTL);
    }
    
    return record;
  }

  public void setKeyBundle(KeyBundle bundle) {
    this.keyBundle = bundle;
  }

  public CryptoRecord decrypt() throws CryptoException, IOException, ParseException,
                       NonObjectJSONException {
    if (keyBundle == null) {
      throw new NoKeyBundleException();
    }

    
    if (!payload.containsKey(KEY_CIPHERTEXT) ||
        !payload.containsKey(KEY_IV) ||
        !payload.containsKey(KEY_HMAC)) {
      throw new MissingCryptoInputException();
    }

    
    
    byte[] cleartext = decryptPayload(payload, keyBundle);
    payload = ExtendedJSONObject.parseUTF8AsJSONObject(cleartext);
    return this;
  }

  public CryptoRecord encrypt() throws CryptoException, UnsupportedEncodingException {
    if (this.keyBundle == null) {
      throw new NoKeyBundleException();
    }
    String cleartext = payload.toJSONString();
    byte[] cleartextBytes = cleartext.getBytes("UTF-8");
    CryptoInfo info = CryptoInfo.encrypt(cleartextBytes, keyBundle);
    String message = new String(Base64.encodeBase64(info.getMessage()));
    String iv      = new String(Base64.encodeBase64(info.getIV()));
    String hmac    = Utils.byte2hex(info.getHMAC());
    ExtendedJSONObject ciphertext = new ExtendedJSONObject();
    ciphertext.put(KEY_CIPHERTEXT, message);
    ciphertext.put(KEY_HMAC, hmac);
    ciphertext.put(KEY_IV, iv);
    this.payload = ciphertext;
    return this;
  }

  @Override
  public void initFromEnvelope(CryptoRecord payload) {
    throw new IllegalStateException("Can't do this with a CryptoRecord.");
  }

  @Override
  public CryptoRecord getEnvelope() {
    throw new IllegalStateException("Can't do this with a CryptoRecord.");
  }

  @Override
  protected void populatePayload(ExtendedJSONObject payload) {
    throw new IllegalStateException("Can't do this with a CryptoRecord.");
  }

  @Override
  protected void initFromPayload(ExtendedJSONObject payload) {
    throw new IllegalStateException("Can't do this with a CryptoRecord.");
  }

  
  public JSONObject toJSONObject() {
    ExtendedJSONObject o = new ExtendedJSONObject();
    o.put(KEY_PAYLOAD, payload.toJSONString());
    o.put(KEY_ID,      this.guid);
    if (this.ttl > 0) {
      o.put(KEY_TTL, this.ttl);
    }
    return o.object;
  }

  @Override
  public String toJSONString() {
    return toJSONObject().toJSONString();
  }
}
