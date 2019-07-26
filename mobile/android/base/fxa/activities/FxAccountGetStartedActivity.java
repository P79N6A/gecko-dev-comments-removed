



package org.mozilla.gecko.fxa.activities;

import java.util.Locale;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountAgeLockoutHelper;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.sync.Utils;
import org.mozilla.gecko.sync.setup.activities.ActivityUtils;
import org.mozilla.gecko.sync.setup.activities.LocaleAware;

import android.accounts.AccountAuthenticatorActivity;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;




public class FxAccountGetStartedActivity extends AccountAuthenticatorActivity {
  protected static final String LOG_TAG = FxAccountGetStartedActivity.class.getSimpleName();

  private static final int CHILD_REQUEST_CODE = 1;

  


  @Override
  public void onCreate(Bundle icicle) {
    Logger.setThreadLogTag(FxAccountConstants.GLOBAL_LOG_TAG);
    Logger.debug(LOG_TAG, "onCreate(" + icicle + ")");

    LocaleAware.initializeLocale(getApplicationContext());

    super.onCreate(icicle);

    setContentView(R.layout.fxaccount_get_started);

    linkifyOldFirefoxLink();

    View button = findViewById(R.id.get_started_button);
    button.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        Intent intent = new Intent(FxAccountGetStartedActivity.this, FxAccountCreateAccountActivity.class);
        
        
        intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        startActivityForResult(intent, CHILD_REQUEST_CODE);
      }
    });
  }

  @Override
  public void onResume() {
    super.onResume();

    Intent intent = null;
    if (FxAccountAgeLockoutHelper.isLockedOut(SystemClock.elapsedRealtime())) {
      intent = new Intent(this, FxAccountCreateAccountNotAllowedActivity.class);
    } else if (FirefoxAccounts.firefoxAccountsExist(this)) {
      intent = new Intent(this, FxAccountStatusActivity.class);
    }

    if (intent != null) {
      this.setAccountAuthenticatorResult(null);
      setResult(RESULT_CANCELED);
      
      
      intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
      this.startActivity(intent);
      this.finish();
    }
  }

  



  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data) {
    Logger.debug(LOG_TAG, "onActivityResult: " + requestCode + ", " + resultCode);
    if (requestCode != CHILD_REQUEST_CODE) {
      super.onActivityResult(requestCode, resultCode, data);
      return;
    }

    this.setResult(requestCode, data);
    if (data != null) {
      this.setAccountAuthenticatorResult(data.getExtras());

      
      
      
      this.finish();
    }
  }

  protected void linkifyOldFirefoxLink() {
    TextView oldFirefox = (TextView) findViewById(R.id.old_firefox);
    String text = getResources().getString(R.string.fxaccount_getting_started_old_firefox);
    String VERSION = AppConstants.MOZ_APP_VERSION;
    String OS = AppConstants.OS_TARGET;

    String LOCALE = Utils.getLanguageTag(Locale.getDefault());
    String url = getResources().getString(R.string.fxaccount_link_old_firefox, VERSION, OS, LOCALE);
    FxAccountConstants.pii(LOG_TAG, "Old Firefox url is: " + url); 
    ActivityUtils.linkTextView(oldFirefox, text, url);
  }
}
