



package org.mozilla.gecko.sync.setup.activities;

import org.mozilla.gecko.R;
import org.mozilla.gecko.sync.setup.Constants;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class SetupFailureActivity extends SyncActivity {
  private Context mContext;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.sync_setup_failure);
    mContext = this.getApplicationContext();

    
    Bundle extras = this.getIntent().getExtras();
    if (extras != null) {
      boolean isAccountError = extras.getBoolean(Constants.INTENT_EXTRA_IS_ACCOUNTERROR);
      if (isAccountError) {
        TextView subtitle1 = (TextView) findViewById(R.id.failure_subtitle1);
        
        
        TextView subtitle2 = (TextView) findViewById(R.id.failure_subtitle2);
        subtitle1.setText(getString(R.string.sync_subtitle_failaccount));
        subtitle2.setVisibility(View.VISIBLE);
        subtitle2.setText(getString(R.string.sync_subtitle_failmultiple));
      }
    }
  }

  public void manualClickHandler(View target) {
    Intent intent = new Intent(mContext, AccountActivity.class);
    intent.setFlags(Constants.FLAG_ACTIVITY_REORDER_TO_FRONT_NO_ANIMATION);
    startActivity(intent);
    overridePendingTransition(0, 0);
    finish();
  }

  public void tryAgainClickHandler(View target) {
    finish();
  }

  public void cancelClickHandler(View target) {
    setResult(RESULT_CANCELED);
    moveTaskToBack(true);
    finish();
  }
}
