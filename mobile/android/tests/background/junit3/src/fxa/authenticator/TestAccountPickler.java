


package org.mozilla.gecko.background.fxa.authenticator;

import org.mozilla.gecko.background.sync.AndroidSyncTestCaseWithAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.authenticator.AccountPickler;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.Separated;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.test.RenamingDelegatingContext;

public class TestAccountPickler extends AndroidSyncTestCaseWithAccounts {
  private static final String TEST_TOKEN_SERVER_URI = "tokenServerURI";
  private static final String TEST_AUTH_SERVER_URI = "serverURI";
  private static final String TEST_PROFILE = "profile";
  private final static String FILENAME_PREFIX = "TestAccountPickler-";
  private final static String PICKLE_FILENAME = "pickle";

  private final static String TEST_ACCOUNTTYPE = FxAccountConstants.ACCOUNT_TYPE;

  
  
  public static final String TEST_USERNAME   = "testFirefoxAccount@mozilla.com";

  public Account account;
  public RenamingDelegatingContext context;

  public TestAccountPickler() {
    super(TEST_ACCOUNTTYPE, TEST_USERNAME);
  }

  @Override
  public void setUp() {
    super.setUp();
    this.account = null;
    
    this.context = new RenamingDelegatingContext(getApplicationContext(), FILENAME_PREFIX +
        Math.random() * 1000001 + "-");
    this.accountManager = AccountManager.get(context);
  }

  @Override
  public void tearDown() {
    super.tearDown();
    this.context.deleteFile(PICKLE_FILENAME);
  }

  public AndroidFxAccount addTestAccount() throws Exception {
    final State state = new Separated(TEST_USERNAME, "uid", false); 
    final AndroidFxAccount account = AndroidFxAccount.addAndroidAccount(context, TEST_USERNAME,
        TEST_PROFILE, TEST_AUTH_SERVER_URI, TEST_TOKEN_SERVER_URI, state);
    assertNotNull(account);
    assertNotNull(account.getProfile());
    assertTrue(testAccountsExist()); 
    this.account = account.getAndroidAccount(); 
    return account;
  }

  public void testPickle() throws Exception {
    final AndroidFxAccount account = addTestAccount();
    
    account.disableSyncing();

    final long now = System.currentTimeMillis();
    final ExtendedJSONObject o = AccountPickler.toJSON(account, now);

    assertEquals(2, o.getLong(AccountPickler.KEY_PICKLE_VERSION).longValue());
    assertTrue(o.getLong(AccountPickler.KEY_PICKLE_TIMESTAMP).longValue() < System.currentTimeMillis());

    assertEquals(AndroidFxAccount.CURRENT_ACCOUNT_VERSION, o.getIntegerSafely(AccountPickler.KEY_ACCOUNT_VERSION).intValue());
    assertEquals(FxAccountConstants.ACCOUNT_TYPE, o.getString(AccountPickler.KEY_ACCOUNT_TYPE));

    assertEquals(TEST_USERNAME, o.getString(AccountPickler.KEY_EMAIL));
    assertEquals(TEST_PROFILE, o.getString(AccountPickler.KEY_PROFILE));
    assertEquals(TEST_AUTH_SERVER_URI, o.getString(AccountPickler.KEY_IDP_SERVER_URI));
    assertEquals(TEST_TOKEN_SERVER_URI, o.getString(AccountPickler.KEY_TOKEN_SERVER_URI));

    assertFalse(o.getBoolean(AccountPickler.KEY_IS_SYNCING_ENABLED));
    assertNotNull(o.get(AccountPickler.KEY_BUNDLE));
  }

  public void testPickleAndUnpickle() throws Exception {
    final AndroidFxAccount inputAccount = addTestAccount();
    
    inputAccount.disableSyncing();

    AccountPickler.pickle(inputAccount, PICKLE_FILENAME);
    final ExtendedJSONObject inputJSON = AccountPickler.toJSON(inputAccount, 0);
    final State inputState = inputAccount.getState();
    assertNotNull(inputJSON);
    assertNotNull(inputState);

    
    deleteTestAccounts();
    assertFalse(testAccountsExist());

    final AndroidFxAccount unpickledAccount = AccountPickler.unpickle(context, PICKLE_FILENAME);
    assertNotNull(unpickledAccount);
    final ExtendedJSONObject unpickledJSON = AccountPickler.toJSON(unpickledAccount, 0);
    final State unpickledState = unpickledAccount.getState();
    assertNotNull(unpickledJSON);
    assertNotNull(unpickledState);

    assertEquals(inputJSON, unpickledJSON);
    assertStateEquals(inputState, unpickledState);
  }

  public void testDeletePickle() throws Exception {
    final AndroidFxAccount account = addTestAccount();
    AccountPickler.pickle(account, PICKLE_FILENAME);

    final String s = Utils.readFile(context, PICKLE_FILENAME);
    assertNotNull(s);
    assertTrue(s.length() > 0);

    AccountPickler.deletePickle(context, PICKLE_FILENAME);
    assertFileNotPresent(context, PICKLE_FILENAME);
  }

  private void assertStateEquals(final State expected, final State actual) throws Exception {
    
    assertEquals(expected.getStateLabel(), actual.getStateLabel());
    assertEquals(expected.email, actual.email);
    assertEquals(expected.uid, actual.uid);
    assertEquals(expected.verified, actual.verified);
  }
}
