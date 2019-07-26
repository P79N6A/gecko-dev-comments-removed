



package org.mozilla.gecko.fxa.authenticator;

import java.io.FileOutputStream;
import java.io.PrintStream;
import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.StateLabel;
import org.mozilla.gecko.fxa.login.StateFactory;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.NonObjectJSONException;
import org.mozilla.gecko.sync.Utils;

import android.content.Context;
































public class AccountPickler {
  public static final String LOG_TAG = AccountPickler.class.getSimpleName();

  public static final long PICKLE_VERSION = 1;

  private static final String KEY_PICKLE_VERSION = "pickle_version";
  private static final String KEY_PICKLE_TIMESTAMP = "pickle_timestamp";

  private static final String KEY_ACCOUNT_VERSION = "account_version";
  private static final String KEY_ACCOUNT_TYPE = "account_type";
  private static final String KEY_EMAIL = "email";
  private static final String KEY_PROFILE = "profile";
  private static final String KEY_IDP_SERVER_URI = "idpServerURI";
  private static final String KEY_TOKEN_SERVER_URI = "tokenServerURI";
  private static final String KEY_IS_SYNCING_ENABLED = "isSyncingEnabled";

  private static final String KEY_BUNDLE = "bundle";

  






  public static boolean deletePickle(final Context context, final String filename) {
    return context.deleteFile(filename);
  }

  





  public static void pickle(final AndroidFxAccount account, final String filename) {
    final ExtendedJSONObject o = new ExtendedJSONObject();
    o.put(KEY_PICKLE_VERSION, Long.valueOf(PICKLE_VERSION));
    o.put(KEY_PICKLE_TIMESTAMP, Long.valueOf(System.currentTimeMillis()));

    o.put(KEY_ACCOUNT_VERSION, AndroidFxAccount.CURRENT_ACCOUNT_VERSION);
    o.put(KEY_ACCOUNT_TYPE, FxAccountConstants.ACCOUNT_TYPE);
    o.put(KEY_EMAIL, account.getEmail());
    o.put(KEY_PROFILE, account.getProfile());
    o.put(KEY_IDP_SERVER_URI, account.getAccountServerURI());
    o.put(KEY_TOKEN_SERVER_URI, account.getTokenServerURI());
    o.put(KEY_IS_SYNCING_ENABLED, account.isSyncingEnabled());

    

    final ExtendedJSONObject bundle = account.unbundle();
    if (bundle == null) {
      Logger.warn(LOG_TAG, "Unable to obtain account bundle; aborting.");
      return;
    }
    o.put(KEY_BUNDLE, bundle);

    writeToDisk(account.context, filename, o);
  }

  private static void writeToDisk(final Context context, final String filename,
      final ExtendedJSONObject pickle) {
    try {
      final FileOutputStream fos = context.openFileOutput(filename, Context.MODE_PRIVATE);
      try {
        final PrintStream ps = new PrintStream(fos);
        try {
          ps.print(pickle.toJSONString());
          Logger.debug(LOG_TAG, "Persisted " + pickle.keySet().size() +
              " account settings to " + filename + ".");
        } finally {
          ps.close();
        }
      } finally {
        fos.close();
      }
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Caught exception persisting account settings to " + filename +
          "; ignoring.", e);
    }
  }

  








  public static AndroidFxAccount unpickle(final Context context, final String filename) {
    final String jsonString = Utils.readFile(context, filename);
    if (jsonString == null) {
      Logger.info(LOG_TAG, "Pickle file '" + filename + "' not found; aborting.");
      return null;
    }

    ExtendedJSONObject json = null;
    try {
      json = ExtendedJSONObject.parseJSONObject(jsonString);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception reading pickle file '" + filename + "'; aborting.", e);
      return null;
    }

    final UnpickleParams params;
    try {
      params = UnpickleParams.fromJSON(json);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Got exception extracting unpickle json; aborting.", e);
      return null;
    }

    final AndroidFxAccount account;
    try {
      account = AndroidFxAccount.addAndroidAccount(context, params.email, params.profile,
          params.idpServerURI, params.tokenServerURI, params.state, params.accountVersion,
          params.isSyncingEnabled, true, params.bundle);
    } catch (Exception e) {
      Logger.warn(LOG_TAG, "Exception when adding Android Account; aborting.", e);
      return null;
    }

    if (account == null) {
      Logger.warn(LOG_TAG, "Failed to add Android Account; aborting.");
      return null;
    }

    Long timestamp = json.getLong(KEY_PICKLE_TIMESTAMP);
    if (timestamp == null) {
      Logger.warn(LOG_TAG, "Did not find timestamp in pickle file; ignoring.");
      timestamp = Long.valueOf(-1);
    }

    Logger.info(LOG_TAG, "Un-pickled Android account named " + params.email + " (version " +
        params.pickleVersion + ", pickled at " + timestamp + ").");

    return account;
  }

  private static class UnpickleParams {
    private Long pickleVersion;

    private int accountVersion;
    private String email;
    private String profile;
    private String idpServerURI;
    private String tokenServerURI;
    private boolean isSyncingEnabled;

    private ExtendedJSONObject bundle;
    private State state;

    private UnpickleParams() {
    }

    private static UnpickleParams fromJSON(final ExtendedJSONObject json)
        throws InvalidKeySpecException, NoSuchAlgorithmException, NonObjectJSONException {
      final UnpickleParams params = new UnpickleParams();
      params.pickleVersion = json.getLong(KEY_PICKLE_VERSION);
      if (params.pickleVersion == null) {
        throw new IllegalStateException("Pickle version not found.");
      }

      switch (params.pickleVersion.intValue()) {
        case 1:
          params.unpickleV1(json);
          break;

        default:
          throw new IllegalStateException("Unknown pickle version, " + params.pickleVersion + ".");
      }

      return params;
    }

    private void unpickleV1(final ExtendedJSONObject json)
        throws NonObjectJSONException, NoSuchAlgorithmException, InvalidKeySpecException {
      
      final String accountType = json.getString(KEY_ACCOUNT_TYPE);
      if (!FxAccountConstants.ACCOUNT_TYPE.equals(accountType)) {
        throw new IllegalStateException("Account type has changed from, " + accountType +
            ", to, " + FxAccountConstants.ACCOUNT_TYPE + ".");
      }

      this.accountVersion = json.getIntegerSafely(KEY_ACCOUNT_VERSION);
      this.email = json.getString(KEY_EMAIL);
      this.profile = json.getString(KEY_PROFILE);
      this.idpServerURI = json.getString(KEY_IDP_SERVER_URI);
      this.tokenServerURI = json.getString(KEY_TOKEN_SERVER_URI);
      this.isSyncingEnabled = json.getBoolean(KEY_IS_SYNCING_ENABLED);

      this.bundle = json.getObject(KEY_BUNDLE);
      if (bundle == null) {
        throw new IllegalStateException("Pickle bundle is null.");
      }
      this.state = getState(bundle);
    }

    private State getState(final ExtendedJSONObject bundle) throws InvalidKeySpecException,
            NonObjectJSONException, NoSuchAlgorithmException {
      
      
      final StateLabel stateLabel = StateLabel.valueOf(
          bundle.getString(AndroidFxAccount.BUNDLE_KEY_STATE_LABEL));
      final String stateString = bundle.getString(AndroidFxAccount.BUNDLE_KEY_STATE);
      if (stateLabel == null) {
        throw new IllegalStateException("stateLabel must not be null");
      }
      if (stateString == null) {
        throw new IllegalStateException("stateString must not be null");
      }

      try {
        return StateFactory.fromJSONObject(stateLabel, new ExtendedJSONObject(stateString));
      } catch (Exception e) {
        throw new IllegalStateException("could not get state", e);
      }
    }
  }
}
