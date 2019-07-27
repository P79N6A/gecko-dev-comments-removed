



package org.mozilla.gecko.fxa.activities;

import java.util.Locale;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.Locales;
import org.mozilla.gecko.R;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.fxa.FxAccountAgeLockoutHelper;
import org.mozilla.gecko.background.fxa.FxAccountUtils;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.FxAccountConstants;
import org.mozilla.gecko.fxa.FxAccountServerConfiguration;
import org.mozilla.gecko.sync.setup.activities.ActivityUtils;

import android.accounts.AccountAuthenticatorActivity;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.TranslateAnimation;
import android.widget.TextView;




public class FxAccountGetStartedActivity extends AccountAuthenticatorActivity {
  protected static final String LOG_TAG = FxAccountGetStartedActivity.class.getSimpleName();

  private static final int CHILD_REQUEST_CODE = 1;

  
  
  @SuppressWarnings("unused")
  private final int NUMBER_OF_CLICKS_TO_LAUNCH_STAGE_FLOW =
      
      (!AppConstants.MOZILLA_OFFICIAL || AppConstants.NIGHTLY_BUILD || AppConstants.DEBUG_BUILD) ? 5 : -1 ;
  private int stageFlowClickCount = 0;

  


  @Override
  public void onCreate(Bundle icicle) {
    Logger.setThreadLogTag(FxAccountConstants.GLOBAL_LOG_TAG);
    Logger.debug(LOG_TAG, "onCreate(" + icicle + ")");

    Locales.initializeLocale(getApplicationContext());

    super.onCreate(icicle);

    setContentView(R.layout.fxaccount_get_started);

    linkifyOldFirefoxLink();

    View button = findViewById(R.id.get_started_button);
    button.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        Bundle extras = null; 
        if (getIntent() != null) {
          extras = getIntent().getExtras();
        }
        startFlow(extras);
      }
    });

    final View iconView = findViewById(R.id.icon);
    animateIconIn(iconView);

    iconView.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        stageFlowClickCount += 1;
        if (NUMBER_OF_CLICKS_TO_LAUNCH_STAGE_FLOW > 0 && stageFlowClickCount >= NUMBER_OF_CLICKS_TO_LAUNCH_STAGE_FLOW) {
          stageFlowClickCount = 0;
          startFlow(FxAccountServerConfiguration.Stage.toBundle());
        }
      }
    });
  }

  









  protected void animateIconIn(View iconView) {
    final AlphaAnimation a = new AlphaAnimation(0.0f, 1.0f);
    final TranslateAnimation t = new TranslateAnimation(
        Animation.RELATIVE_TO_SELF, 0.0f, Animation.RELATIVE_TO_SELF, 0.0f,
        Animation.RELATIVE_TO_SELF, 2.0f, Animation.RELATIVE_TO_SELF, 0.0f);

    final AnimationSet animationSet = new AnimationSet(true);
    animationSet.setDuration(150 * 7); 
    animationSet.addAnimation(a);
    animationSet.addAnimation(t);

    iconView.startAnimation(animationSet);
  }

  protected void startFlow(Bundle extras) {
    final Intent intent = new Intent(this, FxAccountCreateAccountActivity.class);
    
    
    intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    if (extras != null) {
        intent.putExtras(extras);
    }
    startActivityForResult(intent, CHILD_REQUEST_CODE);
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

    
    
    
    
    Bundle extras = null;
    if (getIntent() != null) {
      extras = getIntent().getExtras();
    }
    if (extras != null && extras.containsKey(FxAccountAbstractSetupActivity.EXTRA_EXTRAS)) {
      getIntent().replaceExtras(Bundle.EMPTY);
      startFlow((Bundle) extras.clone());
      return;
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
    final String url = FirefoxAccounts.getOldSyncUpgradeURL(getResources(), Locale.getDefault());
    FxAccountUtils.pii(LOG_TAG, "Old Firefox url is: " + url); 
    ActivityUtils.linkTextView(oldFirefox, text, url);
  }
}
