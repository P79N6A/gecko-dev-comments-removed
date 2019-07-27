



package org.mozilla.gecko.fxa.activities;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountAgeLockoutHelper;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;
import org.mozilla.gecko.sync.setup.activities.ActivityUtils;
import org.mozilla.gecko.LocaleAware.LocaleAwareActivity;

import android.accounts.Account;
import android.app.Activity;
import android.content.Intent;
import android.os.SystemClock;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

public abstract class FxAccountAbstractActivity extends LocaleAwareActivity {
  private static final String LOG_TAG = FxAccountAbstractActivity.class.getSimpleName();

  protected final boolean cannotResumeWhenAccountsExist;
  protected final boolean cannotResumeWhenNoAccountsExist;
  protected final boolean cannotResumeWhenLockedOut;

  public static final int CAN_ALWAYS_RESUME = 0;
  public static final int CANNOT_RESUME_WHEN_ACCOUNTS_EXIST = 1 << 0;
  public static final int CANNOT_RESUME_WHEN_NO_ACCOUNTS_EXIST = 1 << 1;
  public static final int CANNOT_RESUME_WHEN_LOCKED_OUT = 1 << 2;

  public FxAccountAbstractActivity(int resume) {
    super();
    this.cannotResumeWhenAccountsExist = 0 != (resume & CANNOT_RESUME_WHEN_ACCOUNTS_EXIST);
    this.cannotResumeWhenNoAccountsExist = 0 != (resume & CANNOT_RESUME_WHEN_NO_ACCOUNTS_EXIST);
    this.cannotResumeWhenLockedOut = 0 != (resume & CANNOT_RESUME_WHEN_LOCKED_OUT);
  }

  





  protected void redirectIfAppropriate() {
    if (cannotResumeWhenAccountsExist || cannotResumeWhenNoAccountsExist) {
      final Account account = FirefoxAccounts.getFirefoxAccount(this);
      if (cannotResumeWhenAccountsExist && account != null) {
        redirectToActivity(FxAccountStatusActivity.class);
        return;
      }
      if (cannotResumeWhenNoAccountsExist && account == null) {
        redirectToActivity(FxAccountGetStartedActivity.class);
        return;
      }
    }
    if (cannotResumeWhenLockedOut) {
      if (FxAccountAgeLockoutHelper.isLockedOut(SystemClock.elapsedRealtime())) {
        this.setResult(RESULT_CANCELED);
        redirectToActivity(FxAccountCreateAccountNotAllowedActivity.class);
        return;
      }
    }
  }

  @Override
  public void onResume() {
    super.onResume();
    redirectIfAppropriate();
  }

  protected void launchActivity(Class<? extends Activity> activityClass) {
    Intent intent = new Intent(this, activityClass);
    
    
    intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    startActivity(intent);
  }

  protected void redirectToActivity(Class<? extends Activity> activityClass) {
    launchActivity(activityClass);
    finish();
  }

  






  public View ensureFindViewById(View v, int id, String description) {
    View view;
    if (v != null) {
      view = v.findViewById(id);
    } else {
      view = findViewById(id);
    }
    if (view == null) {
      String message = "Could not find view " + description + ".";
      Logger.error(LOG_TAG, message);
      throw new RuntimeException(message);
    }
    return view;
  }

  public void linkifyTextViews(View view, int[] textViews) {
    for (int id : textViews) {
      TextView textView;
      if (view != null) {
        textView = (TextView) view.findViewById(id);
      } else {
        textView = (TextView) findViewById(id);
      }

      if (textView == null) {
        Logger.warn(LOG_TAG, "Could not process links for view with id " + id + ".");
        continue;
      }

      ActivityUtils.linkifyTextView(textView, false);
    }
  }

  protected void launchActivityOnClick(final View view, final Class<? extends Activity> activityClass) {
    view.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        FxAccountAbstractActivity.this.launchActivity(activityClass);
      }
    });
  }

  


  protected AndroidFxAccount getAndroidFxAccount() {
    Account account = FirefoxAccounts.getFirefoxAccount(this);
    if (account == null) {
      return null;
    }
    return new AndroidFxAccount(this, account);
  }
}
