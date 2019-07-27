



package org.mozilla.gecko.fxa.login;

import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.browserid.BrowserIDKeyPair;
import org.mozilla.gecko.browserid.DSACryptoImplementation;
import org.mozilla.gecko.browserid.RSACryptoImplementation;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.login.State.StateLabel;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.Utils;











public class StateFactory {
  private static final String LOG_TAG = StateFactory.class.getSimpleName();

  private static final int KEY_PAIR_SIZE_IN_BITS_V1 = 1024;

  public static BrowserIDKeyPair generateKeyPair() throws NoSuchAlgorithmException {
    
    return DSACryptoImplementation.generateKeyPair(KEY_PAIR_SIZE_IN_BITS_V1);
  }

  protected static BrowserIDKeyPair keyPairFromJSONObjectV1(ExtendedJSONObject o) throws InvalidKeySpecException, NoSuchAlgorithmException {
    
    return RSACryptoImplementation.fromJSONObject(o);
  }

  protected static BrowserIDKeyPair keyPairFromJSONObjectV2(ExtendedJSONObject o) throws InvalidKeySpecException, NoSuchAlgorithmException {
    
    return DSACryptoImplementation.fromJSONObject(o);
  }

  public static State fromJSONObject(StateLabel stateLabel, ExtendedJSONObject o) throws InvalidKeySpecException, NoSuchAlgorithmException, NonObjectJSONException {
    Long version = o.getLong("version");
    if (version == null) {
      throw new IllegalStateException("version must not be null");
    }

    final int v = version.intValue();
    if (v == 2) {
      
      return fromJSONObjectV2(stateLabel, o);
    }
    if (v == 1) {
      final State state = fromJSONObjectV1(stateLabel, o);
      return migrateV1toV2(stateLabel, state);
    }
    throw new IllegalStateException("version must be in {1, 2}");
  }

  protected static State fromJSONObjectV1(StateLabel stateLabel, ExtendedJSONObject o) throws InvalidKeySpecException, NoSuchAlgorithmException, NonObjectJSONException {
    switch (stateLabel) {
    case Engaged:
      return new Engaged(
          o.getString("email"),
          o.getString("uid"),
          o.getBoolean("verified"),
          Utils.hex2Byte(o.getString("unwrapkB")),
          Utils.hex2Byte(o.getString("sessionToken")),
          Utils.hex2Byte(o.getString("keyFetchToken")));
    case Cohabiting:
      return new Cohabiting(
          o.getString("email"),
          o.getString("uid"),
          Utils.hex2Byte(o.getString("sessionToken")),
          Utils.hex2Byte(o.getString("kA")),
          Utils.hex2Byte(o.getString("kB")),
          keyPairFromJSONObjectV1(o.getObject("keyPair")));
    case Married:
      return new Married(
          o.getString("email"),
          o.getString("uid"),
          Utils.hex2Byte(o.getString("sessionToken")),
          Utils.hex2Byte(o.getString("kA")),
          Utils.hex2Byte(o.getString("kB")),
          keyPairFromJSONObjectV1(o.getObject("keyPair")),
          o.getString("certificate"));
    case Separated:
      return new Separated(
          o.getString("email"),
          o.getString("uid"),
          o.getBoolean("verified"));
    case Doghouse:
      return new Doghouse(
          o.getString("email"),
          o.getString("uid"),
          o.getBoolean("verified"));
    default:
      throw new IllegalStateException("unrecognized state label: " + stateLabel);
    }
  }

  


  protected static State fromJSONObjectV2(StateLabel stateLabel, ExtendedJSONObject o) throws InvalidKeySpecException, NoSuchAlgorithmException, NonObjectJSONException {
    switch (stateLabel) {
    case Cohabiting:
      return new Cohabiting(
          o.getString("email"),
          o.getString("uid"),
          Utils.hex2Byte(o.getString("sessionToken")),
          Utils.hex2Byte(o.getString("kA")),
          Utils.hex2Byte(o.getString("kB")),
          keyPairFromJSONObjectV2(o.getObject("keyPair")));
    case Married:
      return new Married(
          o.getString("email"),
          o.getString("uid"),
          Utils.hex2Byte(o.getString("sessionToken")),
          Utils.hex2Byte(o.getString("kA")),
          Utils.hex2Byte(o.getString("kB")),
          keyPairFromJSONObjectV2(o.getObject("keyPair")),
          o.getString("certificate"));
    default:
      return fromJSONObjectV1(stateLabel, o);
    }
  }

  protected static void logMigration(State from, State to) {
    if (!FxAccountConstants.LOG_PERSONAL_INFORMATION) {
      return;
    }
    try {
      FxAccountConstants.pii(LOG_TAG, "V1 persisted state is: " + from.toJSONObject().toJSONString());
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Error producing JSON representation of V1 state.", e);
    }
    FxAccountConstants.pii(LOG_TAG, "Generated new V2 state: " + to.toJSONObject().toJSONString());
  }

  protected static State migrateV1toV2(StateLabel stateLabel, State state) throws NoSuchAlgorithmException {
    if (state == null) {
      
      Logger.error(LOG_TAG, "Got null state in migrateV1toV2; returning null.");
      return state;
    }

    Logger.info(LOG_TAG, "Migrating V1 persisted State to V2; stateLabel: " + stateLabel);

    
    
    
    switch (stateLabel) {
    case Cohabiting: {
      
      final Cohabiting cohabiting = (Cohabiting) state;
      final BrowserIDKeyPair keyPair = generateKeyPair();
      final State migrated = new Cohabiting(cohabiting.email, cohabiting.uid, cohabiting.sessionToken, cohabiting.kA, cohabiting.kB, keyPair);
      logMigration(cohabiting, migrated);
      return migrated;
    }
    case Married: {
      
      
      
      
      final Married married = (Married) state;
      final BrowserIDKeyPair keyPair = generateKeyPair();
      final State migrated = new Cohabiting(married.email, married.uid, married.sessionToken, married.kA, married.kB, keyPair);
      logMigration(married, migrated);
      return migrated;
    }
    default:
      
      return state;
    }
  }
}
