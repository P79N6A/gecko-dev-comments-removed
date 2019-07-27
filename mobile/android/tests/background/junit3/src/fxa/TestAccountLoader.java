















package org.mozilla.gecko.background.fxa;

import java.io.UnsupportedEncodingException;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.atomic.AtomicReference;

import org.mozilla.gecko.background.sync.AndroidSyncTestCaseWithAccounts;
import org.mozilla.gecko.background.sync.TestSyncAccounts;
import org.mozilla.gecko.fxa.AccountLoader;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.Separated;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.sync.SyncConstants;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.sync.setup.SyncAccounts.SyncAccountParameters;

import android.accounts.Account;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.v4.content.Loader;
import android.support.v4.content.Loader.OnLoadCompleteListener;






public class TestAccountLoader extends AndroidSyncTestCaseWithAccounts {
  
  
  private static final String TEST_USERNAME = "testAccount@mozilla.com";
  private static final String TEST_ACCOUNTTYPE = FxAccountConstants.ACCOUNT_TYPE;

  private static final String TEST_SYNCKEY = "testSyncKey";
  private static final String TEST_SYNCPASSWORD = "testSyncPassword";

  private static final String TEST_TOKEN_SERVER_URI = "testTokenServerURI";
  private static final String TEST_AUTH_SERVER_URI = "testAuthServerURI";
  private static final String TEST_PROFILE = "testProfile";

  public TestAccountLoader() {
    super(TEST_ACCOUNTTYPE, TEST_USERNAME);
  }

  static {
    
    
    
    
    
    new AsyncTask<Void, Void, Void>() {
      @Override
      protected Void doInBackground(Void... args) {
        return null;
      }

      @Override
      protected void onPostExecute(Void result) {
      }
    };
  }

  






  public <T> T getLoaderResultSynchronously(final Loader<T> loader) {
    
    final ArrayBlockingQueue<AtomicReference<T>> queue = new ArrayBlockingQueue<AtomicReference<T>>(1);

    
    
    final OnLoadCompleteListener<T> listener = new OnLoadCompleteListener<T>() {
      @Override
      public void onLoadComplete(Loader<T> completedLoader, T data) {
        
        completedLoader.unregisterListener(this);
        completedLoader.stopLoading();
        completedLoader.reset();
        
        queue.add(new AtomicReference<T>(data));
      }
    };

    
    
    
    final Handler mainThreadHandler = new Handler(Looper.getMainLooper()) {
      @Override
      public void handleMessage(Message msg) {
        loader.registerListener(0, listener);
        loader.startLoading();
      }
    };

    
    mainThreadHandler.sendEmptyMessage(0);

    
    T result;
    while (true) {
      try {
        result = queue.take().get();
        break;
      } catch (InterruptedException e) {
        throw new RuntimeException("waiting thread interrupted", e);
      }
    }
    return result;
  }

  public void testInitialLoad() throws UnsupportedEncodingException, GeneralSecurityException, URISyntaxException {
    
    
    
    

    final Context context = getApplicationContext();
    final AccountLoader loader = new AccountLoader(context);

    final boolean syncAccountsExist = SyncAccounts.syncAccountsExist(context);
    final boolean firefoxAccountsExist = FirefoxAccounts.firefoxAccountsExist(context);

    if (firefoxAccountsExist) {
      assertFirefoxAccount(getLoaderResultSynchronously(loader));
      return;
    }

    if (syncAccountsExist) {
      assertSyncAccount(getLoaderResultSynchronously(loader));
      return;
    }

    
    
    Account syncAccount = null;
    try {
      final SyncAccountParameters syncAccountParameters =
          new SyncAccountParameters(context, null, TEST_USERNAME, TEST_SYNCKEY, TEST_SYNCPASSWORD, null);
      syncAccount = SyncAccounts.createSyncAccount(syncAccountParameters, false);
      assertNotNull(syncAccount);
      assertSyncAccount(getLoaderResultSynchronously(loader));
    } finally {
      if (syncAccount != null) {
        TestSyncAccounts.deleteAccount(this, accountManager, syncAccount);
      }
    }

    
    final State state = new Separated(TEST_USERNAME, "uid", false); 
    final AndroidFxAccount account = AndroidFxAccount.addAndroidAccount(context,
        TEST_USERNAME, TEST_PROFILE, TEST_AUTH_SERVER_URI, TEST_TOKEN_SERVER_URI, state);
    assertNotNull(account);
    assertFirefoxAccount(getLoaderResultSynchronously(loader));
  }

  protected void assertFirefoxAccount(Account account) {
    assertNotNull(account);
    assertEquals(FxAccountConstants.ACCOUNT_TYPE, account.type);
  }

  protected void assertSyncAccount(Account account) {
    assertNotNull(account);
    assertEquals(SyncConstants.ACCOUNTTYPE_SYNC, account.type);
  }
}
