



package org.mozilla.gecko.fxa.receivers;

import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.fxa.login.State.StateLabel;
import org.mozilla.gecko.sync.Utils;

import android.accounts.Account;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;




public class FxAccountUpgradeReceiver extends BroadcastReceiver {
  private static final String LOG_TAG = FxAccountUpgradeReceiver.class.getSimpleName();

  









  protected List<Runnable> onUpgradeRunnables(Context context) {
    List<Runnable> runnables = new LinkedList<Runnable>();
    runnables.add(new MaybeUnpickleRunnable(context));
    
    
    runnables.add(new AdvanceFromDoghouseRunnable(context));
    return runnables;
  }

  @Override
  public void onReceive(final Context context, Intent intent) {
    Logger.setThreadLogTag(FxAccountConstants.GLOBAL_LOG_TAG);
    Logger.info(LOG_TAG, "Upgrade broadcast received.");

    
    final Executor executor = Executors.newSingleThreadExecutor();
    for (final Runnable runnable : onUpgradeRunnables(context)) {
      executor.execute(new Runnable() {
        @Override
        public void run() {
          try {
            runnable.run();
          } catch (Exception e) {
            
            
            Logger.error(LOG_TAG, "Got exception executing background upgrade Runnable; ignoring.", e);
          }
        }
      });
    }
  }

  


  protected static class MaybeUnpickleRunnable implements Runnable {
    protected final Context context;

    public MaybeUnpickleRunnable(Context context) {
      this.context = context;
    }

    @Override
    public void run() {
      
      Logger.info(LOG_TAG, "Trying to unpickle any pickled Firefox Account.");
      FirefoxAccounts.getFirefoxAccounts(context);
    }
  }

  








  protected static class AdvanceFromDoghouseRunnable implements Runnable {
    protected final Context context;

    public AdvanceFromDoghouseRunnable(Context context) {
      this.context = context;
    }

    @Override
    public void run() {
      final Account[] accounts = FirefoxAccounts.getFirefoxAccounts(context);
      Logger.info(LOG_TAG, "Trying to advance " + accounts.length + " existing Firefox Accounts from the Doghouse to Separated (if necessary).");
      for (Account account : accounts) {
        try {
          final AndroidFxAccount fxAccount = new AndroidFxAccount(context, account);
          
          if (FxAccountUtils.LOG_PERSONAL_INFORMATION) {
            fxAccount.dump();
          }
          State state = fxAccount.getState();
          if (state == null || state.getStateLabel() != StateLabel.Doghouse) {
            Logger.debug(LOG_TAG, "Account named like " + Utils.obfuscateEmail(account.name) + " is not in the Doghouse; skipping.");
            continue;
          }
          Logger.debug(LOG_TAG, "Account named like " + Utils.obfuscateEmail(account.name) + " is in the Doghouse; advancing to Separated.");
          fxAccount.setState(state.makeSeparatedState());
        } catch (Exception e) {
          Logger.warn(LOG_TAG, "Got exception trying to advance account named like " + Utils.obfuscateEmail(account.name) +
              " from Doghouse to Separated state; ignoring.", e);
        }
      }
    }
  }
}
