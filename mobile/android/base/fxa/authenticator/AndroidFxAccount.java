



package org.mozilla.gecko.fxa.authenticator;

import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;
import java.util.ArrayList;
import java.util.Collections;

import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.browserid.BrowserIDKeyPair;
import org.mozilla.gecko.browserid.RSACryptoImplementation;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;
import android.os.Bundle;









public class AndroidFxAccount implements AbstractFxAccount {
  protected static final String LOG_TAG = AndroidFxAccount.class.getSimpleName();

  public static final int CURRENT_PREFS_VERSION = 1;

  public static final int CURRENT_ACCOUNT_VERSION = 3;
  public static final String ACCOUNT_KEY_ACCOUNT_VERSION = "version";
  public static final String ACCOUNT_KEY_PROFILE = "profile";
  public static final String ACCOUNT_KEY_IDP_SERVER = "idpServerURI";

  
  public static final String ACCOUNT_KEY_AUDIENCE = "audience";                 
  public static final String ACCOUNT_KEY_TOKEN_SERVER = "tokenServerURI";       
  public static final String ACCOUNT_KEY_DESCRIPTOR = "descriptor";

  public static final int CURRENT_BUNDLE_VERSION = 1;
  public static final String BUNDLE_KEY_BUNDLE_VERSION = "version";
  public static final String BUNDLE_KEY_ASSERTION = "assertion";
  public static final String BUNDLE_KEY_CERTIFICATE = "certificate";
  public static final String BUNDLE_KEY_INVALID = "invalid";
  public static final String BUNDLE_KEY_SESSION_TOKEN = "sessionToken";
  public static final String BUNDLE_KEY_KEY_FETCH_TOKEN = "keyFetchToken";
  public static final String BUNDLE_KEY_VERIFIED = "verified";
  public static final String BUNDLE_KEY_KA = "kA";
  public static final String BUNDLE_KEY_KB = "kB";
  public static final String BUNDLE_KEY_UNWRAPKB = "unwrapkB";
  public static final String BUNDLE_KEY_ASSERTION_KEY_PAIR = "assertionKeyPair";

  protected final Context context;
  protected final AccountManager accountManager;
  protected final Account account;

  















  public AndroidFxAccount(Context applicationContext, Account account) {
    this.context = applicationContext;
    this.account = account;
    this.accountManager = AccountManager.get(this.context);
  }

  protected int getAccountVersion() {
    String v = accountManager.getUserData(account, ACCOUNT_KEY_ACCOUNT_VERSION);
    if (v == null) {
      return 0;         
    }

    try {
      return Integer.parseInt(v, 10);
    } catch (NumberFormatException ex) {
      return 0;
    }
  }

  protected void persistBundle(ExtendedJSONObject bundle) {
    accountManager.setUserData(account, ACCOUNT_KEY_DESCRIPTOR, bundle.toJSONString());
  }

  protected ExtendedJSONObject unbundle() {
    final int version = getAccountVersion();
    if (version < CURRENT_ACCOUNT_VERSION) {
      
      return null;
    }

    if (version > CURRENT_ACCOUNT_VERSION) {
      
      return null;
    }

    String bundle = accountManager.getUserData(account, ACCOUNT_KEY_DESCRIPTOR);
    if (bundle == null) {
      return null;
    }
    return unbundleAccountV1(bundle);
  }

  protected String getBundleData(String key) {
    ExtendedJSONObject o = unbundle();
    if (o == null) {
      return null;
    }
    return o.getString(key);
  }

  protected boolean getBundleDataBoolean(String key, boolean def) {
    ExtendedJSONObject o = unbundle();
    if (o == null) {
      return def;
    }
    Boolean b = o.getBoolean(key);
    if (b == null) {
      return def;
    }
    return b.booleanValue();
  }

  protected byte[] getBundleDataBytes(String key) {
    ExtendedJSONObject o = unbundle();
    if (o == null) {
      return null;
    }
    return o.getByteArrayHex(key);
  }

  protected void updateBundleDataBytes(String key, byte[] value) {
    updateBundleValue(key, value == null ? null : Utils.byte2Hex(value));
  }

  protected void updateBundleValue(String key, boolean value) {
    ExtendedJSONObject descriptor = unbundle();
    if (descriptor == null) {
      return;
    }
    descriptor.put(key, value);
    persistBundle(descriptor);
  }

  protected void updateBundleValue(String key, String value) {
    ExtendedJSONObject descriptor = unbundle();
    if (descriptor == null) {
      return;
    }
    descriptor.put(key, value);
    persistBundle(descriptor);
  }

  private ExtendedJSONObject unbundleAccountV1(String bundle) {
    ExtendedJSONObject o;
    try {
      o = new ExtendedJSONObject(bundle);
    } catch (Exception e) {
      return null;
    }
    if (CURRENT_BUNDLE_VERSION == o.getIntegerSafely(BUNDLE_KEY_BUNDLE_VERSION)) {
      return o;
    }
    return null;
  }

  @Override
  public byte[] getEmailUTF8() {
    try {
      return account.name.getBytes("UTF-8");
    } catch (UnsupportedEncodingException e) {
      
      return null;
    }
  }

  



  @Override
  public String getProfile() {
    return accountManager.getUserData(account, ACCOUNT_KEY_PROFILE);
  }


  @Override
  public String getAccountServerURI() {
    return accountManager.getUserData(account, ACCOUNT_KEY_IDP_SERVER);
  }

  public String getAudience() {
    return accountManager.getUserData(account, ACCOUNT_KEY_AUDIENCE);
  }

  public String getTokenServerURI() {
    return accountManager.getUserData(account, ACCOUNT_KEY_TOKEN_SERVER);
  }

  


  public String getSyncPrefsPath() throws GeneralSecurityException, UnsupportedEncodingException {
    String profile = getProfile();
    String username = account.name;

    if (profile == null ||
        username == null) {
      throw new IllegalStateException("Missing profile or username. Cannot fetch prefs.");
    }

    final String tokenServerURI = getTokenServerURI();
    if (tokenServerURI == null) {
      throw new IllegalStateException("No token server URI. Cannot fetch prefs.");
    }

    final String fxaServerURI = getAccountServerURI();
    if (fxaServerURI == null) {
      throw new IllegalStateException("No account server URI. Cannot fetch prefs.");
    }

    final String product = GlobalConstants.BROWSER_INTENT_PACKAGE + ".fxa";
    final long version = CURRENT_PREFS_VERSION;

    
    final String serverURLThing = fxaServerURI + "!" + tokenServerURI;
    return Utils.getPrefsPath(product, username, serverURLThing, profile, version);
  }

  @Override
  public void setQuickStretchedPW(byte[] quickStretchedPW) {
    accountManager.setPassword(account, quickStretchedPW == null ? null : Utils.byte2Hex(quickStretchedPW));
  }


  @Override
  public byte[] getQuickStretchedPW() {
    String quickStretchedPW = accountManager.getPassword(account);
    return quickStretchedPW == null ? null : Utils.hex2Byte(quickStretchedPW);
  }

  protected byte[] getUserDataBytes(String key) {
    String data = accountManager.getUserData(account, key);
    if (data == null) {
      return null;
    }
    return Utils.hex2Byte(data);
  }

  @Override
  public byte[] getSessionToken() {
    return getBundleDataBytes(BUNDLE_KEY_SESSION_TOKEN);
  }

  @Override
  public byte[] getKeyFetchToken() {
    return getBundleDataBytes(BUNDLE_KEY_KEY_FETCH_TOKEN);
  }

  @Override
  public void setSessionToken(byte[] sessionToken) {
    updateBundleDataBytes(BUNDLE_KEY_SESSION_TOKEN, sessionToken);
  }

  @Override
  public void setKeyFetchToken(byte[] keyFetchToken) {
    updateBundleDataBytes(BUNDLE_KEY_KEY_FETCH_TOKEN, keyFetchToken);
  }

  @Override
  public boolean isVerified() {
    return getBundleDataBoolean(BUNDLE_KEY_VERIFIED, false);
  }

  @Override
  public void setVerified() {
    updateBundleValue(BUNDLE_KEY_VERIFIED, true);
  }

  @Override
  public byte[] getKa() {
    return getUserDataBytes(BUNDLE_KEY_KA);
  }

  @Override
  public void setKa(byte[] kA) {
    updateBundleValue(BUNDLE_KEY_KA, Utils.byte2Hex(kA));
  }

  @Override
  public void setWrappedKb(byte[] wrappedKb) {
    byte[] unwrapKb = getUserDataBytes(BUNDLE_KEY_UNWRAPKB);
    byte[] kB = new byte[wrappedKb.length]; 
    for (int i = 0; i < wrappedKb.length; i++) {
      kB[i] = (byte) (wrappedKb[i] ^ unwrapKb[i]);
    }
    updateBundleValue(BUNDLE_KEY_KB, Utils.byte2Hex(kB));
  }

  @Override
  public byte[] getKb() {
    return getUserDataBytes(BUNDLE_KEY_KB);
  }

  protected BrowserIDKeyPair generateNewAssertionKeyPair() throws GeneralSecurityException {
    Logger.info(LOG_TAG, "Generating new assertion key pair.");
    
    return RSACryptoImplementation.generateKeyPair(1024);
  }

  @Override
  public BrowserIDKeyPair getAssertionKeyPair() throws GeneralSecurityException {
    try {
      String data = getBundleData(BUNDLE_KEY_ASSERTION_KEY_PAIR);
      return RSACryptoImplementation.fromJSONObject(new ExtendedJSONObject(data));
    } catch (Exception e) {
      
    }

    BrowserIDKeyPair keyPair = generateNewAssertionKeyPair();

    ExtendedJSONObject descriptor = unbundle();
    if (descriptor == null) {
      descriptor = new ExtendedJSONObject();
    }
    descriptor.put(BUNDLE_KEY_ASSERTION_KEY_PAIR, keyPair.toJSONObject().toJSONString());
    persistBundle(descriptor);
    return keyPair;
  }

  @Override
  public String getCertificate() {
    return getBundleData(BUNDLE_KEY_CERTIFICATE);
  }

  @Override
  public void setCertificate(String certificate) {
    updateBundleValue(BUNDLE_KEY_CERTIFICATE, certificate);
  }

  @Override
  public String getAssertion() {
    return getBundleData(BUNDLE_KEY_ASSERTION);
  }

  @Override
  public void setAssertion(String assertion) {
    updateBundleValue(BUNDLE_KEY_ASSERTION, assertion);
  }

  








  public ExtendedJSONObject toJSONObject() {
    ExtendedJSONObject o = unbundle();
    o.put("email", account.name);
    try {
      o.put("emailUTF8", Utils.byte2Hex(account.name.getBytes("UTF-8")));
    } catch (UnsupportedEncodingException e) {
      
    }
    o.put("quickStretchedPW", accountManager.getPassword(account));
    return o;
  }

  public static Account addAndroidAccount(
      Context context,
      String email,
      String password,
      String profile,
      String idpServerURI,
      String tokenServerURI,
      byte[] sessionToken,
      byte[] keyFetchToken,
      boolean verified)
          throws UnsupportedEncodingException, GeneralSecurityException, URISyntaxException {
    if (email == null) {
      throw new IllegalArgumentException("email must not be null");
    }
    if (password == null) {
      throw new IllegalArgumentException("password must not be null");
    }
    if (idpServerURI == null) {
      throw new IllegalArgumentException("idpServerURI must not be null");
    }
    if (tokenServerURI == null) {
      throw new IllegalArgumentException("tokenServerURI must not be null");
    }
    
    
    
    
    if ((sessionToken == null && keyFetchToken != null) ||
        (sessionToken != null && keyFetchToken == null)) {
      throw new IllegalArgumentException("none or both of sessionToken and keyFetchToken may be null");
    }

    byte[] emailUTF8 = email.getBytes("UTF-8");
    byte[] passwordUTF8 = password.getBytes("UTF-8");
    byte[] quickStretchedPW = FxAccountUtils.generateQuickStretchedPW(emailUTF8, passwordUTF8);
    byte[] unwrapBkey = FxAccountUtils.generateUnwrapBKey(quickStretchedPW);

    Bundle userdata = new Bundle();
    userdata.putInt(ACCOUNT_KEY_ACCOUNT_VERSION, CURRENT_ACCOUNT_VERSION);
    userdata.putString(ACCOUNT_KEY_IDP_SERVER, idpServerURI);
    userdata.putString(ACCOUNT_KEY_TOKEN_SERVER, tokenServerURI);
    userdata.putString(ACCOUNT_KEY_AUDIENCE, computeAudience(tokenServerURI));

    ExtendedJSONObject descriptor = new ExtendedJSONObject();
    descriptor.put(BUNDLE_KEY_BUNDLE_VERSION, CURRENT_BUNDLE_VERSION);
    descriptor.put(BUNDLE_KEY_SESSION_TOKEN, sessionToken == null ? null : Utils.byte2Hex(sessionToken));
    descriptor.put(BUNDLE_KEY_KEY_FETCH_TOKEN, keyFetchToken == null ? null : Utils.byte2Hex(keyFetchToken));
    descriptor.put(BUNDLE_KEY_VERIFIED, Boolean.valueOf(verified).toString());
    descriptor.put(BUNDLE_KEY_UNWRAPKB, Utils.byte2Hex(unwrapBkey));

    userdata.putString(ACCOUNT_KEY_DESCRIPTOR, descriptor.toJSONString());

    Account account = new Account(email, FxAccountConstants.ACCOUNT_TYPE);
    AccountManager accountManager = AccountManager.get(context);
    boolean added = accountManager.addAccountExplicitly(account, Utils.byte2Hex(quickStretchedPW), userdata);
    if (!added) {
      return null;
    }
    FxAccountAuthenticator.enableSyncing(context, account);
    return account;
  }

  
  private static String computeAudience(String tokenServerURI) throws URISyntaxException {
     URI uri = new URI(tokenServerURI);
     return new URI(uri.getScheme(), uri.getHost(), null, null).toString();
  }

  @Override
  public boolean isValid() {
    
    
    return !getBundleDataBoolean(BUNDLE_KEY_INVALID, false);
  }

  @Override
  public void setInvalid() {
    updateBundleValue(BUNDLE_KEY_INVALID, true);
  }

  


  public void dump() {
    if (!FxAccountConstants.LOG_PERSONAL_INFORMATION) {
      return;
    }
    ExtendedJSONObject o = toJSONObject();
    ArrayList<String> list = new ArrayList<String>(o.keySet());
    Collections.sort(list);
    for (String key : list) {
      FxAccountConstants.pii(LOG_TAG, key + ": " + o.getString(key));
    }
  }

  


  public void forgetAccountTokens() {
    ExtendedJSONObject descriptor = unbundle();
    if (descriptor == null) {
      return;
    }
    descriptor.remove(BUNDLE_KEY_SESSION_TOKEN);
    descriptor.remove(BUNDLE_KEY_KEY_FETCH_TOKEN);
    persistBundle(descriptor);
  }

  


  public void forgetQuickstretchedPW() {
    accountManager.setPassword(account, null);
  }
}
